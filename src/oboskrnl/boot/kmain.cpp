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

#include <boot/cfg.h>

#define LITERAL(str) (char*)(str), sizeof((str))

extern obos::memory::VirtualAllocator g_liballocVirtualAllocator;


namespace obos
{
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
		// Reconstruct the liballoc's allocator with the kernel process, to avoid future problems with liballoc in user mode.
		g_liballocVirtualAllocator.~VirtualAllocator();
		new (&g_liballocVirtualAllocator) memory::VirtualAllocator{ kernelProc };
		process::g_processes.tail = process::g_processes.head = kernelProc;
		kernelProc->pid = process::g_processes.size++;
		
		logger::log("Loading the initrd driver.\n");
		SetLastError(0);
		if (!driverInterface::LoadModule(initrdDriverData, initrdDriverSize, nullptr))
			logger::panic(nullptr, "Could not load the initrd driver. GetLastError: %d\n", GetLastError());
		SetLastError(0);

		new (&vfs::g_mountPoints) utils::Vector<vfs::MountPoint*>{};

		logger::log("Mounting the initrd.\n");
		uint32_t point = 0;
		// Mount the initrd.
		if (!vfs::mount(point, 0))
			logger::panic(nullptr, "Could not mount the initrd, GetLastError: %d!\n", GetLastError());

		// Load all modules under the initrd.
		driverInterface::ScanAndLoadModules("0:/");

		// Load the kernel configuration file.
		logger::log("Loading 0:/kcfg.cfg\n");
		vfs::FileHandle kcfgFile;
		Parser kcfg;
		if (!kcfgFile.Open("0:/kcfg.cfg"))
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
		logger::log("Successfully loaded and parsed 0:/kcfg.cfg\n");
		if (!kcfg.GetElement("FS_DRIVERS"))
			logger::panic(nullptr, "Missing required property in 0:/kcfg.cfg:FS_DRIVERS.\n");
		const utils::Vector<Element>& fs_drivers = kcfg.GetElement("FS_DRIVERS")->array;
		if (!fs_drivers.length())
			logger::panic(nullptr, "No filesystem drivers listed in 0:/kfg.cfg:FS_DRIVERS.\n");
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
			char* fdata = new char[fdrv.GetFileSize()];
			fdrv.Read(fdata, fdrv.GetFileSize());
			fdrv.Close();
			driverInterface::driverHeader* header = driverInterface::CheckModule((byte*)fdata, fdrv.GetFileSize());
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
			if (!driverInterface::LoadModule((byte*)fdata, fdrv.GetFileSize(), nullptr))
			{
				fdrv.Close();
				delete[] fdata;
				logger::warning("Could not load driver %s.\n", driverPath.data());
				continue;
			}
			nFSDriversLoaded++;
		}
		if (!nFSDriversLoaded)
			logger::panic(nullptr, "No file system drivers loaded!\n");
		
		thread::ExitThread(0);
	}
}