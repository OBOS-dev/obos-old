/*
	oboskrnl/vfs/mount/mount.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <utils/vector.h>

#include <vfs/vfsNode.h>

namespace obos
{
	namespace vfs
	{
		extern utils::Vector<MountPoint*> g_mountPoints;
		/// <summary>
		/// Mounts a partition. You can mount a partition multiple times.
		/// </summary>
		/// <param name="point">The place to store the mount id where the partition was mounted. If this is 0xffffffff, the function will choose a mount point.</param>
		/// <param name="driveId">The drive that contains the partition id to mount.</param>
		/// <param name="partitionId">The partition id to mount.</param>
		/// <param name="isInitrd">Whether the initrd should be mounted at point or not.</param>
		/// <param name="failIfPartitionHasMountPoint">If an existing mount point is found, and this parameter is true, "point" will be set to the mount point, and the function will fail.
		/// </param>
		/// <returns>Whether the function succeeded (true) or not (false)</returns>
		bool mount(uint32_t& point, uint32_t driveId, uint32_t partitionId, bool isInitrd = false, bool failIfPartitionHasMountPoint = false);
		/// <summary>
		/// Unmounts the mount point. This will invalidate any file handles.
		/// </summary>
		/// <param name="point">The mount point to unmount.</param>
		/// <returns>Whether the function succeeded (true) or not (false)</returns>
		bool unmount(uint32_t point);

		/// <summary>
		/// Gets the partition id for the mount point.
		/// </summary>
		/// <param name="mountPoint">The mount point to query.</param>
		/// <returns>UINT64_MAX on failure, otherwise the top 32-bits are the drive id, and the bottom 32-bits are the partition id</returns>
		uint64_t getPartitionIDForMountPoint(uint32_t mountPoint);

		/// <summary>
		/// Gets the mount points for a partition id.
		/// </summary>
		/// <param name="driveId">The drive that the partition is on.</param>
		/// <param name="partitionId">The partition id to find.</param>
		/// <param name="oMountPoints">A pointer to an array allocated by the function containing all the mount points that use partition id.</param>
		void getMountPointsForPartitionID(uint32_t driveId, uint32_t partitionId, uint32_t** oMountPoints);
	}
}