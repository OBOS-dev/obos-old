/*
	oboskrnl/boot/kmain.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

// __ALL__ code in this file must be platform-independent.

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

#if defined(__x86_64__) || defined(_WIN64)
#include <multitasking/process/x86_64/loader/elf.h>
#include <arch/x86_64/irq/irq.h>
namespace obos
{
	bool EnterSleepState(int state);
}
#endif

#define LITERAL(str) (char*)(str), sizeof((str))

extern obos::memory::VirtualAllocator g_liballocVirtualAllocator;

namespace obos
{
	thread::ThreadHandle kBootThread;
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

		kBootThread.OpenThread(thread::GetTID());

		logger::log("%s: Loading the initrd driver.\n", __func__);
		SetLastError(0);
		if (!driverInterface::LoadModule(initrdDriverData, initrdDriverSize, nullptr))
			logger::panic(nullptr, "Could not load the initrd driver. GetLastError: %d\n", GetLastError());
		SetLastError(0);

		new (&vfs::g_mountPoints) utils::Vector<vfs::MountPoint*>{};

		logger::log("%s: Mounting the initrd.\n", __func__);
		uint32_t point = 0;
		// Mount the initrd.
		if (!vfs::mount(point, 0, 0, true))
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
			logger::log("Loading filesystem driver %s.\n", driverPath.data());
			if (!driverInterface::LoadModule((byte*)fdata, drvFilesize, nullptr))
			{
				fdrv.Close();
				delete[] fdata;
				logger::warning("Could not load driver %s.\n", driverPath.data());
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
				char* partPath = new char[logger::sprintf(nullptr, "%*sP%d:/", dPath.length(), dPath.data(), _part)];
				logger::sprintf(partPath, "%*sP%d:/", dPath.length(), dPath.data(), _part);
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
					if (GetLastError() != OBOS_ERROR_VFS_PARTITION_ALREADY_MOUNTED)
						logger::warning("%s: Could not mount partition. GetLastError: %d\n", __func__, GetLastError());
					else
						logger::warning("%s: Could not mount partition as the partition is already mounted. Mount point: %d\n", __func__, mountPoint);
					part.Close();
					delete[] partPath;
					continue;
				}
				logger::log("%s: Mounted partition %s at %d:/.\n", __func__, partPath, mountPoint);
				delete[] partPath;
				part.Close();
			}
		}
		if (!kcfg.GetElement("INIT_PROGRAM"))
			logger::panic(nullptr, "Missing required property \"INIT_PROGRAM\" in 0:/boot.cfg.\n");
		const utils::String& initProgramPath = kcfg.GetElement("INIT_PROGRAM")->string;
		logger::log("%s: Attempting load of init program '%s'.\n", __func__, initProgramPath.data());
		vfs::FileHandle handle;
		if (!handle.Open(initProgramPath.data(), vfs::FileHandle::OPTIONS_READ_ONLY))
			logger::panic(nullptr, "%s: Open(): Could not find init program '%s'. GetLastError: %d.\n", __func__, initProgramPath.data(), GetLastError());
		//utils::String initProgramContents;
		//initProgramContents.resize(handle.GetFileSize());
		//if (!handle.Read(initProgramContents.data(), initProgramContents.length(), false))
		//	logger::panic(nullptr, "%s: While calling Read(): Could not load init program '%s'. GetLastError: %d.\n", __func__, initProgramPath.data(), GetLastError());
		char* initProgramContents = (char*)memory::VirtualAllocator{ nullptr }.VirtualMapFile(nullptr, handle.GetFileSize(), 0, &handle, memory::PROT_READ_ONLY);
		process::Process* proc = process::CreateProcess(true);
		uintptr_t entry = 0;
		uintptr_t baseAddress = 0;
		// Loading programs into memory is unfortunately arch-specific.
#if defined(__x86_64__) || defined(_WIN64)
		using namespace process::loader;
		if (LoadElfFile((byte*)initProgramContents, handle.GetFileSize(), entry, baseAddress, proc->vallocator, false) != 0)
			logger::panic(nullptr, "%s: While calling LoadElfFile(), Could not load '%s'. GetLastError: %d.\n", __func__, initProgramPath.data(), GetLastError());
#endif
		thread::ThreadHandle initProgramMainThread;
		char* program_arguments = (char*)proc->vallocator.Memcpy(proc->vallocator.VirtualAlloc(nullptr, initProgramPath.length() + 1, memory::PROT_USER_MODE_ACCESS), initProgramPath.data(), initProgramPath.length());
		initProgramMainThread.CreateThread(
			4,
			0x10000,
			(void(*)(uintptr_t))entry,
			(uintptr_t)program_arguments,
			thread::g_defaultAffinity,
			proc,
			true
		);
		thread::Thread* _initProgramMainThread = (thread::Thread*)initProgramMainThread.GetUnderlyingObject();
		_initProgramMainThread->blockCallback.callback = [](thread::Thread*, void* udata)
			{
				thread::ThreadHandle* kBootThread = (thread::ThreadHandle*)udata;
				if (kBootThread->GetThreadStatus() == thread::THREAD_STATUS_DEAD)
				{
					kBootThread->CloseHandle();
					return true;
				}
				return false;
			};
		_initProgramMainThread->blockCallback.userdata = &kBootThread;
		_initProgramMainThread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_BLOCKED;
		initProgramMainThread.CloseHandle();
		memory::VirtualAllocator{ nullptr }.VirtualFree(initProgramContents, handle.GetFileSize());
		handle.Close();

		logger::log("%s: Done early-kernel boot process!\n", __func__);
		thread::ExitThread(0);
	}
}