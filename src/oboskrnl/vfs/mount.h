/*
	vfs/mount.h

	Copyright (c) 2023 Omar Berrow
*/
#pragma once

#include <types.h>

#include <vfs/node.h>

namespace obos
{
	namespace vfs
	{
		DWORD MountDrive(UINT16_T mountPoint, mountFlags flags, SIZE_T filesystemDriverId, UINTPTR_T drive);
		DWORD UnmountDrive(UINT16_T mountPoint);
	}
}