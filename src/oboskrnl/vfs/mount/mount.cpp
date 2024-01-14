/*
	oboskrnl/vfs/mount/mount.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <vector.h>
#include <klog.h>

#include <vfs/mount/mount.h>

#include <vfs/vfsNode.h>

#include <vfs/devManip/driveHandle.h>

#include <driverInterface/struct.h>
#include <driverInterface/load.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace vfs
	{
		utils::Vector<MountPoint*> g_mountPoints;
		// Don't make static!
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
						tokens[nTokensCounted++] = (const char*)utils::memcpy(new char[utils::strCountToChar(filepath + i + 1, '/') + 2], filepath + i + (i != 0), utils::strCountToChar(filepath + i + (i != 0), '/') + 1);
					else
						tokens[nTokensCounted++] = filepath + i + (i != 0);
				}
			}
		}
		static bool setupMountPointEntries(MountPoint* point)
		{
			if (point->filesystemDriver->_serviceType != ((point->partitionId == 0) + driverInterface::OBOS_SERVICE_TYPE_FILESYSTEM))
			{
				SetLastError(OBOS_ERROR_VFS_NOT_A_FILESYSTEM_DRIVER);
				return false;
			}
			auto functions = point->filesystemDriver->functionTable.serviceSpecific.filesystem;
			uint32_t driveId = point->partitionId >> 24;
			uint8_t drivePartitionId = point->partitionId & 0xff;
			uintptr_t fileIterator = 0;
			if (!functions.FileIteratorCreate(driveId, drivePartitionId, &fileIterator))
			{
				SetLastError(OBOS_ERROR_VFS_DRIVER_FAILURE);
				return false;
			}
			// Iterate over the files, adding new directory entries.
			size_t filesize = 0;
			driverInterface::fileAttributes fAttributes;
			const char* filepath = nullptr;
			void(*freeFilepath)(void* buff) = nullptr;
			while (1)
			{
				if (!functions.FileIteratorNext(fileIterator, &filepath, &freeFilepath, &filesize, &fAttributes))
				{
					SetLastError(OBOS_ERROR_VFS_DRIVER_FAILURE);
					return false;
				}
 				if (fAttributes == driverInterface::FILE_DOESNT_EXIST)
					break;
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
						size_t cFilesize = filesize;
						driverInterface::fileAttributes cFAttributes = fAttributes;
						if(!(fAttributes & driverInterface::FILE_ATTRIBUTES_DIRECTORY))
						{
							// If the entry we're making isn't a file we must check if the current part of the path we're parsing
							// is a directory or not, otherwise we might make a file entry for a directory.
							if (!functions.QueryFileProperties(sTokens[i], driveId, drivePartitionId, &cFilesize, &cFAttributes))
							{
								for (size_t j = 0; j < nTokens; j++)
									delete[] sTokens[j];
								delete[] tokens;
								delete[] sTokens;
								delete[] filepath;
								SetLastError(OBOS_ERROR_VFS_DRIVER_FAILURE);
								return false;
							}
						}
						if(cFAttributes & driverInterface::FILE_ATTRIBUTES_DIRECTORY)
							directoryEntry = new Directory{};
						else
							directoryEntry = new DirectoryEntry{};
						directoryEntry->path = (char*)utils::memcpy(new char[utils::strlen(sTokens[i]) + 1], sTokens[i], utils::strlen(sTokens[i]));
						directoryEntry->fileAttrib = cFAttributes;
						directoryEntry->filesize = cFilesize;
						if (point->children.tail)
							point->children.tail->next = directoryEntry;
						if (!point->children.head)
							point->children.head = directoryEntry;
						directoryEntry->prev = point->children.tail;
						directoryEntry->mountPoint = point;
						point->children.tail = directoryEntry;
						point->children.size++;
						if (cFAttributes & driverInterface::FILE_ATTRIBUTES_FILE)
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
					size_t cFilesize = filesize;
						driverInterface::fileAttributes cFAttributes = fAttributes;
						if(!(fAttributes & driverInterface::FILE_ATTRIBUTES_DIRECTORY))
						{
							if (!functions.QueryFileProperties(path, driveId, drivePartitionId, &cFilesize, &cFAttributes))
							{
								for (size_t j = 0; j < nTokens; j++)
									delete[] sTokens[j];
								delete[] tokens;
								delete[] sTokens;
								delete[] filepath;
								SetLastError(OBOS_ERROR_VFS_DRIVER_FAILURE);
								return false;
							}
						}
					if (cFAttributes & driverInterface::FILE_ATTRIBUTES_DIRECTORY)
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
					newDirectoryEntry->fileAttrib = fAttributes;
					newDirectoryEntry->filesize = cFilesize;

					directoryEntry = newDirectoryEntry;
				}

				for (size_t j = 0; j < nTokens; j++)
					delete[] sTokens[j];
				delete[] sTokens;
				delete[] tokens;
				freeFilepath((void*)filepath);
				filepath = nullptr;
			}

			// Close the file iterator.
			if (!functions.FileIteratorClose(fileIterator))
			{
				SetLastError(OBOS_ERROR_VFS_DRIVER_FAILURE);
				return false;
			}

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
			utils::memzero(newPoint, sizeof(*newPoint));
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
					newPoint->filesystemDriver = driverInterface::g_driverInterfaces.at(0);
				else 
				{
					uint32_t driveId = newPoint->partitionId >> 24;
					uint8_t drivePartitionId = newPoint->partitionId & 0xff;
					DriveHandle drv;
					char* path = new char[
						logger::sprintf(nullptr, "D%dP%d://", driveId, drivePartitionId) + 1
					];
					logger::sprintf(path, "D%dP%d://", driveId, drivePartitionId);
					if (!drv.OpenDrive(path, DriveHandle::OPTIONS_READ_ONLY))
					{
						delete newPoint;
						delete[] path;
						return false;
					}
					delete[] path;
					newPoint->filesystemDriver = ((PartitionEntry*)drv.GetNode())->filesystemDriver;
					if (!newPoint->filesystemDriver)
					{
						delete newPoint;
						return false;
					}
					drv.Close();
				}
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
			// TODO: Implement.
			SetLastError(OBOS_ERROR_UNIMPLEMENTED_FEATURE);
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
			utils::Vector<uint32_t> mPoints;
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