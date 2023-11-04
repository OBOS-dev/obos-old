/*
	oboskrnl/vfs/mount/mount.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <vector.h>
#include <klog.h>

#include <vfs/mount/mount.h>
#include <vfs/vfsNode.h>

#include <driverInterface/interface.h>
#include <driverInterface/struct.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace vfs
	{
		Vector<MountPoint*> g_mountPoints;

		bool SendCommand(driverInterface::DriverClient& client, uint32_t command)
		{
			if (!client.SendData(&command, sizeof(command)))
			{
				client.CloseConnection();
				return false;
			}
			return true;
		}
		bool NextFile(driverInterface::DriverClient& client, uintptr_t fileIterator, uint64_t*& fileAttribs, char*& filepath)
		{
			if (!SendCommand(client, driverInterface::OBOS_SERVICE_NEXT_FILE))
				return false;
			if (!client.SendData(&fileIterator, sizeof(fileIterator)))
			{
				client.CloseConnection();
				return false;
			}
			if (!client.RecvData(fileAttribs, sizeof(uint64_t) * 3, 15000))
			{
				client.CloseConnection();
				return false;
			}
			size_t filepathSize = 0;
			if (!client.RecvData(&filepathSize, sizeof(filepathSize), 15000))
			{
				client.CloseConnection();
				return false;
			}
			filepath = new char[filepathSize + 1];
			if (!client.RecvData(filepath, filepathSize, 15000))
			{
				client.CloseConnection();
				return false;
			}
			return true;
		}
		bool GetFileAttributes(driverInterface::DriverClient& client, const char* path, uint32_t partitionId, uint64_t*& fileAttribs)
		{
			if (!SendCommand(client, driverInterface::OBOS_SERVICE_QUERY_FILE_DATA))
				return false;
			size_t filepathLen = utils::strlen(path);
			if (!client.SendData(&filepathLen, sizeof(filepathLen)))
			{
				client.CloseConnection();
				return false;
			}
			if (!client.SendData(path, filepathLen))
			{
				client.CloseConnection();
				return false;
			}
			uint64_t driveId = partitionId >> 24;
			if (!client.SendData(&driveId, sizeof(driveId)))
			{
				client.CloseConnection();
				return false;
			}
			byte driverPartitionId = partitionId & 0xff;
			if (!client.SendData(&driverPartitionId, sizeof(driverPartitionId)))
			{
				client.CloseConnection();
				return false;
			}
			if (!client.RecvData(fileAttribs, sizeof(uint64_t) * 3, 15000))
			{
				client.CloseConnection();
				return false;
			}
			return true;
		}
		void dividePathToTokens(const char* filepath, const char**& tokens, size_t& nTokens, bool useOffset = true)
		{
			for (; filepath[0] == '/'; filepath++);
			nTokens = 1;
			for (size_t i = 0; filepath[i]; i++)
				nTokens += filepath[i] == '/';
			for (int i = utils::strlen(filepath) - 1; filepath[i] == '/'; i--)
				nTokens--;
			tokens = (const char**)(new char* [nTokens]);
			tokens[0] = filepath;
			for (size_t i = 0, nTokensCounted = 0; filepath[i] && nTokensCounted < nTokens; i++)
			{
				if (filepath[i] == '/' || i == 0)
				{
					if (!useOffset)
					{
						size_t filepathLen = utils::strlen(filepath);
						((char*)filepath)[filepathLen] = '/'; // So strCountToChar doesn't go off the string.
						tokens[nTokensCounted++] = (const char*)utils::memcpy(new char[utils::strCountToChar(filepath + i + 1, '/') + 1], filepath + i + (i != 0), utils::strCountToChar(filepath + i + (i != 0), '/'));
						((char*)filepath)[filepathLen] = 0;
					}
					else
						tokens[nTokensCounted++] = filepath + i + (i != 0);
				}
			}
		}
		bool setupMountPointEntries(MountPoint* point)
		{
			process::Process* proc = (process::Process*)point->filesystemDriver->process;
			driverInterface::DriverClient client{};
			if (!client.OpenConnection(proc->pid, 15000)) // Connect to the driver.
				return false;
			// Check if the driver is a filesystem (or initrd filesystem if pid == 1) driver.
			{
				if (!SendCommand(client, driverInterface::OBOS_SERVICE_GET_SERVICE_TYPE))
					return false;
				uint32_t response = 0;
				if (!client.RecvData(&response, sizeof(response), 15000))
				{
					client.CloseConnection();
					return false;
				}
				if (response != driverInterface::OBOS_SERVICE_TYPE_FILESYSTEM + (proc->pid == 1))
				{
					SetLastError(OBOS_ERROR_VFS_NOT_A_FILESYSTEM_DRIVER);
					client.CloseConnection();
					return false;
				}
			}
			// Make a file iterator.
			if (!SendCommand(client, driverInterface::OBOS_SERVICE_MAKE_FILE_ITERATOR))
				return false;
			client.SendData(nullptr, 9);
			uintptr_t fileIterator = 0;
			if (!client.RecvData(&fileIterator, sizeof(fileIterator), 15000))
			{
				client.CloseConnection();
				return false;
			}
			// Iterate over the files, adding new directory entries.
			uint64_t fileAttribs[3] = {};
			char* filepath = nullptr;
			while (1)
			{
				{
					uint64_t* _fileAttribs = (uintptr_t*)&fileAttribs[0];
					if (!NextFile(client, fileIterator, _fileAttribs, filepath))
					{
						if (filepath)
							delete[] filepath;
						return false;
					}
				}
				if (utils::memcmp(&fileAttribs[0], (uint32_t)0, sizeof(fileAttribs)))
				{
					if (filepath)
						delete[] filepath;
					break;
				}
				const char** tokens = nullptr;
				size_t nTokens = 0;
				dividePathToTokens(filepath, tokens, nTokens, false);
				DirectoryEntry* directoryEntry = nullptr;
				for(size_t i = 0; i < nTokens; i++)
				{
					if (i == 0)
					{
						if (point->head)
						{
							for (DirectoryEntry* currentEntry = point->head; currentEntry;)
							{
								if (utils::strcmp(currentEntry->path.str, tokens[i]))
								{
									directoryEntry = currentEntry;
									break;
								}

								currentEntry = currentEntry->next;
							}
						}
						if (!directoryEntry)
						{
							uint64_t pathAttrib[3] = {};
							uint64_t *_pathAttrib = &pathAttrib[0];
							if (!GetFileAttributes(client, tokens[0], point->partitionId, _pathAttrib))
							{
								for (size_t j = 0; j < nTokens; j++)
									delete[] tokens[j];
								delete[] tokens;
								delete[] filepath;
								return false;
							}
							if (pathAttrib[0] & driverInterface::FILE_ATTRIBUTES_DIRECTORY)
							{
								directoryEntry = new Directory{};
								Directory* directory = (Directory*)directoryEntry;
								if (point->tail)
									point->tail->next = directory;
								if (!point->head)
									point->head = directory;
								directory->next = nullptr;
								directory->prev = point->tail;
								point->tail = directory;
								point->nDirectories++;
								directory->fileAttrib = (driverInterface::fileAttributes)(pathAttrib[0] & ~driverInterface::FILE_ATTRIBUTES_DIRECTORY);
								directory->path.str = (char*)tokens[i];
								directory->path.strLen = utils::strlen(tokens[i]);
							}
							else
								directoryEntry = new DirectoryEntry{};
						}
					}
				}
				filepath = nullptr;
			}

			// Close the file iterator.
			if (!SendCommand(client, driverInterface::OBOS_SERVICE_CLOSE_FILE_ITERATOR))
				return false;

			return true;
		}
		bool mount(uint32_t& point, uint32_t partitionId, bool failIfPartitionHasMountPoint)
		{
			if (point != 0xffffffff && point < g_mountPoints.length())
			{
				if (g_mountPoints[point])
				{
					SetLastError(OBOS_ERROR_VFS_ALREADY_MOUNTED);
					return false;
				}
			}
			if (point == 0xffffffff)
				point = g_mountPoints.length();
			MountPoint* existingMountPoint = nullptr;
			for(size_t i = 0; i < g_mountPoints.length(); i++)
			{
				if (g_mountPoints[i])
				{
					if (g_mountPoints[i]->partitionId == partitionId)
					{
						existingMountPoint = g_mountPoints[i];
						break;
					}
				}
			}
			if (existingMountPoint && failIfPartitionHasMountPoint)
			{
				SetLastError(OBOS_ERROR_VFS_PARTITION_ALREADY_MOUNTED);
				return false;
			}
			MountPoint* newPoint = new MountPoint;
			newPoint->id = point;
			newPoint->partitionId = partitionId;
			if (existingMountPoint)
			{
				newPoint->filesystemDriver = existingMountPoint->filesystemDriver;
				newPoint->children = existingMountPoint->children;
				newPoint->countChildren = existingMountPoint->countChildren;
				newPoint->otherMountPointsReferencing++;
			}
			else
			{
				if (partitionId == 0)
					newPoint->filesystemDriver = (driverInterface::driverIdentity*)process::g_processes.head->next->_driverIdentity;
				else {} // TODO: Find the filesystem driver associated with the partition.
				bool ret = setupMountPointEntries(newPoint);
				if (ret)
					g_mountPoints.push_back(newPoint);
				return ret;
			}
			g_mountPoints.push_back(newPoint);
			return true;
		}
		bool unmount(uint32_t /*mountPoint*/)
		{
			return false;
		}

		uint32_t getPartitionIDForMountPoint(uint32_t /*mountPoint*/)
		{
			return 0;
		}

		void getMountPointsForPartitionID(uint32_t /*partitionId*/, uint32_t** /*oMountPoints*/)
		{

		}
	}
}