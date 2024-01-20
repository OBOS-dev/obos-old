/*
	oboskrnl/boot/kmain.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

// __ALL__ code in this file must be platform-independent.

#if defined(__x86_64__) || defined(_WIN64)
#include <limine.h>
#endif

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>
#include <error.h>

#include <new>

#include <multitasking/scheduler.h>
#include <multitasking/thread.h>
#include <multitasking/cpu_local.h>

#include <multitasking/process/process.h>

#include <driverInterface/load.h>

#include <vfs/mount/mount.h>

#include <vfs/fileManip/fileHandle.h>

#include <vfs/devManip/driveHandle.h>
#include <vfs/devManip/driveIterator.h>

#include <vfs/vfsNode.h>

#include <boot/cfg.h>

#define LITERAL(str) (char*)(str), sizeof((str))

extern obos::memory::VirtualAllocator g_liballocVirtualAllocator;

namespace obos
{
	namespace vfs
	{
		DirectoryEntry* SearchForNode(DirectoryEntry* root, void* userdata, bool(*compare)(DirectoryEntry* current, void* userdata));
	}
	void kmain_common(byte* initrdDriverData, size_t initrdDriverSize)
	{
		logger::log("Multitasking initialized! In \"%s\" now.\n", __func__);
		process::Process* kernelProc = new process::Process{};
		auto currentThread = thread::GetCurrentCpuLocalPtr()->currentThread;
		kernelProc->console = &g_kernelConsole;
		kernelProc->isUsermode = false;
		kernelProc->parent = nullptr;
		kernelProc->threads.head = currentThread->threadList->head;
		kernelProc->threads.tail = currentThread->threadList->tail;
		kernelProc->threads.size = currentThread->threadList->size;
		thread::Thread* thread = currentThread->next_list;
		while (thread)
		{
			thread->owner = kernelProc;
			thread->threadList = &kernelProc->threads;
		
			thread = thread->next_list;
		}
		delete currentThread->threadList;
		currentThread->threadList = &kernelProc->threads;
		currentThread->owner = kernelProc;	
#if defined(__x86_64__) || defined(_WIN64)
		kernelProc->context.cr3 = currentThread->context.cr3; // Only this time... (reference to line 7)
#endif
		process::g_processes.tail = process::g_processes.head = kernelProc;
		kernelProc->pid = process::g_processes.size++;

		// Reconstruct the liballoc's allocator with the kernel process, to avoid future problems with liballoc in user mode.
		g_liballocVirtualAllocator.~VirtualAllocator();
		new (&g_liballocVirtualAllocator) memory::VirtualAllocator{ kernelProc };

		logger::log("%s: Loading the initrd driver.\n", __func__);
		SetLastError(0);
		if (!driverInterface::LoadModule(initrdDriverData, initrdDriverSize, nullptr))
			logger::panic(nullptr, "Could not load the initrd driver. GetLastError: %d\n", GetLastError());
		SetLastError(0);

		new (&vfs::g_mountPoints) utils::Vector<vfs::MountPoint*>{};

		logger::log("%s: Mounting the initrd.\n", __func__);
		uint32_t point = 0;
		// Mount the initrd.
		if (!vfs::mount(point, 0,0, true))
			logger::panic(nullptr, "Could not mount the initrd, GetLastError: %d!\n", GetLastError());

		// Load all modules under the initrd.
		driverInterface::ScanAndLoadModules("0:/");

		// Load the kernel configuration file.
		logger::log("Loading 0:/boot.cfg\n");
		vfs::FileHandle kcfgFile;
		Parser kcfg;
		if (!kcfgFile.Open("0:/boot.cfg"))
			logger::panic(nullptr, "Could not find the kernel configuration file!\n");
		size_t kcfgFsize = kcfgFile.GetFileSize();
		char* kcfgData = new char[kcfgFsize];
		kcfgFile.Read(kcfgData, kcfgFsize);
		kcfgFile.Close();
		utils::Vector<const char*> errorMessages;
		if (!kcfg.Parse(kcfgData, kcfgFsize, errorMessages))
		{
			// Put all the error messages into one big buffer.
			size_t szErrorMessages = 0;
			for (auto msg : errorMessages)
				szErrorMessages += utils::strlen(msg) + 1;
			char* emsg = new char[szErrorMessages + 1];
			size_t currentOffset = 0;
			for (auto msg : errorMessages)
			{
				utils::memcpy(emsg + currentOffset, msg, utils::strlen(msg));
				currentOffset += utils::strlen(msg);
				*(emsg + currentOffset) = '\n';
				currentOffset++;
			}
			logger::panic(nullptr, "Could not parse kernel config file! Parser output:\n%s", emsg);
		}
		logger::log("%s: Successfully loaded and parsed 0:/boot.cfg\n", __func__);
		if (!kcfg.GetElement("FS_DRIVERS"))
			logger::panic(nullptr, "Missing required property \"FS_DRIVERS\" in 0:/boot.cfg.\n");
		const utils::Vector<Element>& fs_drivers = kcfg.GetElement("FS_DRIVERS")->array;
		if (!fs_drivers.length())
			logger::panic(nullptr, "No filesystem drivers listed in 0:/boot.cfg:FS_DRIVERS.\n");
		size_t nFSDriversLoaded = 0;
		for (const auto& ele : fs_drivers)
		{
			if (ele.type != Element::ELEMENT_STRING)
			{
				logger::warning("Element in FS_DRIVERS is not a string, continuing...\n");
				continue;
			}
			const utils::String& driverPath = ele.string;
			vfs::FileHandle fdrv;
			if (!fdrv.Open(driverPath.data()))
			{
				logger::warning("Could not load driver %s. GetLastError: %d\n", driverPath.data(), GetLastError());
				continue;
			}
			size_t drvFilesize = fdrv.GetFileSize();
			char* fdata = new char[drvFilesize];
			fdrv.Read(fdata, drvFilesize);
			fdrv.Close();
			driverInterface::driverHeader* header = driverInterface::CheckModule((byte*)fdata, drvFilesize);
			if (!header)
			{
				fdrv.Close();
				delete[] fdata;
				logger::warning("Ignoring non-driver file %s.\n", driverPath.data());
				continue;
			}
			if (header->driverType != driverInterface::OBOS_SERVICE_TYPE_FILESYSTEM)
			{
				fdrv.Close();
				delete[] fdata;
				logger::warning("Ignoring driver %s of type %d.\n", driverPath, header->driverType);
				continue;
			}
			logger::log("Loading filesystem driver %S.\n", driverPath);
			if (!driverInterface::LoadModule((byte*)fdata, drvFilesize, nullptr))
			{
				fdrv.Close();
				delete[] fdata;
				logger::warning("Could not load driver %S.\n", driverPath);
				continue;
			}
			delete[] fdata;
			nFSDriversLoaded++;
		}
		if (!nFSDriversLoaded)
			logger::panic(nullptr, "No file system drivers loaded!\n");
		// Mount partitions.
		logger::log("%s: Mounting partitions!\n", __func__);
		for (vfs::DriveIterator drvIter; drvIter; )
		{
			const char* path = drvIter++;
			vfs::DriveHandle drv, part;
			drv.OpenDrive(path, vfs::DriveHandle::OPTIONS_READ_ONLY);
			size_t nPartitions = 0;
			drv.QueryInfo(nullptr, nullptr, &nPartitions);
			utils::String dPath;
			dPath.initialize(path, utils::strlen(path) - 2);
			delete[] path;
			for (size_t _part = 0; _part < nPartitions; _part++)
			{
				char* partPath = new char[logger::sprintf(nullptr, "%SP%d:/", dPath, _part)];
				logger::sprintf(partPath, "%SP%d:/", dPath, _part);
				part.OpenDrive(partPath);
				const char* fsName = nullptr;
				part.QueryPartitionInfo(nullptr, nullptr, &fsName);
				logger::log("%s: Attempting mount of partition %s.\n", __func__, partPath);
				if (utils::strcmp(fsName, "UNKNOWN"))
				{
					logger::warning("%s: Partition has unknown filesystem, skipping mount.\n", __func__);
					part.Close();
					delete[] partPath;
					continue;
				}
				logger::log("%s: Partition filesystem type: %s\n", __func__, fsName);
				uint32_t mountPoint = 0xffffffff;
				if (!vfs::mount(mountPoint, drv.GetDriveId(), _part, false, true))
				{
					logger::warning("%s: Could not mount partition. GetLastError: %d\n", __func__, GetLastError());
					part.Close();
					delete[] partPath;
					continue;
				}
				logger::log("%s: Mounted partition %s at %d:/.\n", __func__, partPath, mountPoint);
				delete[] partPath;
				part.Close();
			}
		}
		// Test the VFS on the FAT32 file.
		[[maybe_unused]]
		auto vfsTestOnPath = [](uint32_t mountPoint, const char* _path)
			{
				char* path = new char[logger::sprintf(nullptr, "%d:/%s", mountPoint, _path) + 1];
				logger::sprintf(path, "%d:/%s", mountPoint, _path);
				vfs::FileHandle file;
				if (!file.Open(path))
				{
					logger::log("Open(%s): GetLastError: %d\n", path, GetLastError());
					delete[] path;
					return false;
				}
				size_t filesize = file.GetFileSize();
				char* data = new char[filesize + 1];
				utils::memzero(data, filesize + 1);
				if (!file.Read(data, filesize))
				{
					logger::log("Read(%s): GetLastError: %d\n", path, GetLastError());
					delete[] path;
					return false;
				}
				file.Close();
				logger::log("Successfully read %s. File contents:\n", path);
				logger::printf("%s\n", data);
				delete[] data;
				delete[] path;
				return true;
			};
		[[maybe_unused]] uint32_t id = 0xffffffff;
		for (auto& mountPoint : vfs::g_mountPoints)
		{
			if (!vfs::SearchForNode(mountPoint->children.head, nullptr, [](vfs::DirectoryEntry* ent, void*)->bool {
				logger::printf("%d:/%s\n", ent->mountPoint->id, ent->path);
				return utils::strcmp(ent->path, "dir") || utils::strcmp(ent->path, "dir/");
				}))
				continue;
			id = mountPoint->id;
			break;
		}
		/*if (id == 0xffffffff)
			goto end;*/
		id = 1;
		vfsTestOnPath(id, "test.txt");
		vfsTestOnPath(id, "dir/other_test.txt");
		vfsTestOnPath(id, "dir/subdirectory/you_guessed_it_another_test.txt");
	[[maybe_unused]] end:
		logger::log("%s: Done early-kernel boot process!\n", __func__);
		thread::ExitThread(0);
	}
}