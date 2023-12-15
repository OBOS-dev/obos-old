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
		bool GetFileAttributes(driverInterface::DriverClient& client, const char* path, uint32_t partitionId, uint64_t* fileAttribs)
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
		bool ReadFile(driverInterface::DriverClient& client, const char* path, uint32_t partitionId, void** data, size_t* nRead, size_t nToSkip, size_t nToRead)
		{
			if (!SendCommand(client, driverInterface::OBOS_SERVICE_READ_FILE))
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
			uint8_t driverPartitionId = partitionId & 0xff;
			if (!client.SendData(&driverPartitionId, sizeof(driverPartitionId)))
			{
				client.CloseConnection();
				return false;
			}
			// sizeof(size_t) is not guarenteed to be sizeof(uint64_t) (see driver_communications.md->OBOS_SERVICE_READ_FILE)
			uint64_t _nToSkip = nToSkip;
			if (!client.SendData(&_nToSkip, sizeof(_nToSkip)))
			{
				client.CloseConnection();
				return false;
			}
			if (!client.SendData(&nToRead, sizeof(nToRead)))
			{
				client.CloseConnection();
				return false;
			}
			size_t _nRead = 0;
			if (!client.RecvData(&_nRead, sizeof(_nRead), 15000))
			{
				client.CloseConnection();
				return false;
			}
			if (nRead)
				*nRead = _nRead;
			void* _data = nullptr;
			if (data)
			{
				_data = new char[_nRead + 1];
				utils::memzero(_data, _nRead + 1);
				*data = _data;
			}
			if (!client.RecvData(data ? _data : nullptr, _nRead, 15000))
			{
				client.CloseConnection();
				return false;
			}
			return true;
		}
		static void dividePathToTokens(const char* filepath, const char**& tokens, size_t& nTokens, bool useOffset = true)
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
						tokens[nTokensCounted++] = (const char*)utils::memcpy(new char[utils::strCountToChar(filepath + i + 1, '/') + 2], filepath + i + (i != 0), utils::strCountToChar(filepath + i + (i != 0), '/') + 1);
					else
						tokens[nTokensCounted++] = filepath + i + (i != 0);
				}
			}
		}
		static bool setupMountPointEntries(MountPoint* point)
		{
			process::Process* proc = (process::Process*)point->filesystemDriver->process;
			driverInterface::DriverClient client{};
			if (!client.OpenConnection(proc->pid, /*60000*/(uintptr_t)-1)) // Connect to the driver.
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
			uint64_t driveId = point->partitionId >> 24;
			if (!client.SendData(&driveId, sizeof(driveId)))
			{
				client.CloseConnection();
				return false;
			}
			uint8_t driverPartitionId = point->partitionId & 0xff;
			if (!client.SendData(&driverPartitionId, sizeof(driverPartitionId)))
			{
				client.CloseConnection();
				return false;
			}
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
				const char** tokens = nullptr, **sTokens = nullptr;
				size_t nTokens = 0;
				dividePathToTokens(filepath, tokens, nTokens);
				dividePathToTokens(filepath, sTokens, nTokens, false);
				DirectoryEntry* directoryEntry = nullptr;
				for(size_t i = 0; i < nTokens; i++)
				{
					if (i == 0)
					{
						directoryEntry = point->children.head;
						while (directoryEntry)
						{
							if (utils::strcmp(directoryEntry->path.str, sTokens[i]))
								break;

							directoryEntry = directoryEntry->next;
						}
						if (directoryEntry)
							continue; // Continue linking the next part of the path
						// We must make a new directory entry.
						// If the entry that we're making is a directory, all of it's tokens will be a directory as well.
						uint64_t attribs[3] = { fileAttribs[0], fileAttribs[1], fileAttribs[2] };
						if(!(attribs[2] & driverInterface::FILE_ATTRIBUTES_DIRECTORY))
						{
							if (!GetFileAttributes(client, sTokens[i], point->partitionId, (uint64_t*)&attribs))
							{
								for (size_t j = 0; j < nTokens; j++)
									delete[] sTokens[j];
								delete[] tokens;
								delete[] sTokens;
								delete[] filepath;
								return false;
							}
						}
						if(attribs[2] & driverInterface::FILE_ATTRIBUTES_DIRECTORY)
							directoryEntry = new Directory{};
						else
							directoryEntry = new DirectoryEntry{};
						directoryEntry->path = (char*)utils::memcpy(new char[utils::strlen(sTokens[i]) + 1], sTokens[i], utils::strlen(sTokens[i]));
						directoryEntry->fileAttrib = attribs[2];
						directoryEntry->filesize = attribs[0];
						if (point->children.tail)
							point->children.tail->next = directoryEntry;
						if (!point->children.head)
							point->children.head = directoryEntry;
						directoryEntry->prev = point->children.tail;
						directoryEntry->mountPoint = point;
						point->children.tail = directoryEntry;
						point->children.size++;
						if (attribs[2] & driverInterface::FILE_ATTRIBUTES_FILE)
							directoryEntry->direntType = DIRECTORY_ENTRY_TYPE_FILE;
						continue;
					}
					DirectoryEntry* newDirectoryEntry = nullptr;
					char* path = nullptr;
					if ((i + 1) == nTokens)
						path = (char*)utils::memcpy(new char[utils::strlen(filepath) + 1], filepath, utils::strlen(filepath));
					else
					{
						// Calculate the size of the path.
						size_t szPath = 1;
						for (size_t j = 0; j < (i + 1); j++)
							szPath += utils::strlen(sTokens[j]) + ((j + 1) != (i + 1)); // The size of the token+'/' if this token isn't the last token
						path = new char[szPath];
						char* iter = path;
						for (size_t j = 0; j < (i + 1); j++)
						{
							size_t szToken = utils::strlen(sTokens[j]);
							utils::memcpy(iter, sTokens[j], szToken);
							iter += szToken;
							if (((j + 1) != (i + 1)) && sTokens[j][szToken - 1] != '/')
								*iter++ = '/';
						}
					}
					if(directoryEntry)
						newDirectoryEntry = directoryEntry->children.head;
					else
						newDirectoryEntry = point->children.head;
					while (newDirectoryEntry)
					{
						if (utils::strcmp(newDirectoryEntry->path.str, path))
							break;

						newDirectoryEntry = newDirectoryEntry->next;
					}
					if (newDirectoryEntry)
					{
						directoryEntry = newDirectoryEntry;
						if(path != filepath)
							delete[] path;
						continue; // Continue linking the next part of the path
					}
					uint64_t attribs[3] = { fileAttribs[0], fileAttribs[1], fileAttribs[2] };
					if (!(attribs[2] & driverInterface::FILE_ATTRIBUTES_DIRECTORY))
					{
						if (!GetFileAttributes(client, path, point->partitionId, (uint64_t*)&attribs))
						{
							for (size_t j = 0; j < nTokens; j++)
								delete[] sTokens[j];
							delete[] tokens;
							delete[] sTokens;
							delete[] filepath;
							if (path != filepath)
								delete[] path;
							return false;
						}
					}
					if (attribs[2] & driverInterface::FILE_ATTRIBUTES_DIRECTORY)
						newDirectoryEntry = new Directory{};
					else
					{
						newDirectoryEntry = new DirectoryEntry{};
						newDirectoryEntry->direntType = DIRECTORY_ENTRY_TYPE_FILE;
					}
					newDirectoryEntry->parent = directoryEntry;
					newDirectoryEntry->path = path;
					if (directoryEntry->children.tail)
						directoryEntry->children.tail->next = newDirectoryEntry;
					if (!directoryEntry->children.head)
						directoryEntry->children.head = newDirectoryEntry;
					newDirectoryEntry->prev = directoryEntry->children.tail;
					newDirectoryEntry->mountPoint = point;
					directoryEntry->children.tail = newDirectoryEntry;
					directoryEntry->children.size++;
					newDirectoryEntry->fileAttrib = attribs[2];
					newDirectoryEntry->filesize = attribs[0];

					directoryEntry = newDirectoryEntry;
				}

				for (size_t j = 0; j < nTokens; j++)
					delete[] sTokens[j];
				delete[] sTokens;
				delete[] tokens;
				delete[] filepath;
				filepath = nullptr;
			}

			// Close the file iterator.
			if (!SendCommand(client, driverInterface::OBOS_SERVICE_CLOSE_FILE_ITERATOR))
				return false;
			if (!client.SendData(&fileIterator, sizeof(fileIterator)))
				return false;
			client.RecvData(nullptr, sizeof(uint8_t));

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
				newPoint->otherMountPointsReferencing++;
			}
			else
			{
				if (partitionId == 0)
					newPoint->filesystemDriver = (driverInterface::driverIdentity*)process::g_processes.head->next->_driverIdentity;
				else 
				{
					SetLastError(OBOS_ERROR_UNIMPLEMENTED_FEATURE);
					return false;
				} // TODO: Find the filesystem driver associated with the partition.
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

		uint32_t getPartitionIDForMountPoint(uint32_t mountPoint)
		{
			MountPoint* _mountPoint = nullptr;
			for (size_t i = 0; i < g_mountPoints.length(); i++)
			{
				if (g_mountPoints[i])
				{
					if (g_mountPoints[i]->id == mountPoint)
					{
						_mountPoint = g_mountPoints[i];
						break;
					}
				}
			}
			if (!_mountPoint)
				return 0;
			return _mountPoint->partitionId;
		}

		void getMountPointsForPartitionID(uint32_t partitionId, uint32_t** oMountPoints)
		{
			if (!oMountPoints)
				return;
			Vector<uint32_t> mPoints;
			for (size_t i = 0; i < g_mountPoints.length(); i++)
			{
				auto mountPoint = g_mountPoints[i];
				if (mountPoint->partitionId == partitionId)
					mPoints.push_back(mountPoint->id);
			}
			if (mPoints.length() == 0)
				return;
			*oMountPoints = new uint32_t[mPoints.length()];
			utils::dwMemcpy(*oMountPoints, mPoints.data(), mPoints.length());
		}
	}
}