/*
	oboskrnl/vfs/fileHandle.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace vfs
	{
		typedef intptr_t off_t;
		typedef uintptr_t uoff_t;
		enum OpenOptions
		{
			OPTIONS_READ_ONLY = 0b1, // Open the file read only.
			OPTIONS_APPEND = 0b10, // Open the file, then seek to the end.
		};
		enum Flags
		{
			//FLAGS_REACHED_EOF = 0b1,
			FLAGS_ALLOW_WRITE = 0b10,
			FLAGS_CLOSED = 0b100,
		};
		enum SeekPlace
		{
			SEEKPLACE_CUR,
			SEEKPLACE_BEG,
			SEEKPLACE_END,
		};
		class FileHandle
		{
		public:
			FileHandle() = default;

			// TODO: Remove the default to options when Write is implemented.
			/// <summary>
			/// Opens a file.
			/// </summary>
			/// <param name="path">The file path.</param>
			/// <param name="options">The open options.</param>
			/// <returns>Whether the file could be opened (true) or not (false). If it fails, use GetLastError for an error code.</returns>
			bool Open(const char* path, OpenOptions options = OPTIONS_READ_ONLY);

			/// <summary>
			/// Reads "nToRead" from the file into "data".
			/// If EOF is reached before all the data could be read, the function fails with OBOS_ERROR_VFS_READ_ABORTED.
			/// </summary>
			/// <param name="data">The buffer to read into.</param>
			/// <param name="nToRead">The count of bytes to read.</param>
			/// <param name="peek">Whether to increment the stream position.</param>
			/// <returns>Whether the file could be read (true) or not (false). If it fails, use GetLastError for an error code.</returns>
			bool Read(char* data, size_t nToRead, bool peek = false);

			bool Write(); // Not implemented.

			/// <summary>
			/// Checks if the file handle has reached the end of the file.
			/// </summary>
			/// <returns>Whether EOF has been reached (true) or not (false)</returns>
			bool Eof() const;
			/// <summary>
			/// Gets the current file offset.
			/// </summary>
			/// <returns>The current file offset.</returns>
			uoff_t GetPos() const { return m_currentFilePos; }
			/// <summary>
			/// Gets the file handle's status/flags.
			/// </summary>
			/// <returns>The handle's status/flags.</returns>
			uint32_t GetFlags() const { return m_flags; }
			/// <summary>
			/// Gets the file's size.
			/// </summary>
			/// <returns>The file's size.</returns>
			size_t GetFileSize() const;
			/// <summary>
			/// Gets the path of the parent directory of the file.
			/// </summary>
			/// <param name="path">The buffer to put the path in. This can be nullptr.</param>
			/// <param name="sizePath">(output) The size of the path.</param>
			void GetParent(char* path, size_t* sizePath);

			/// <summary>
			/// Seeks to count based on the parameters.
			/// </summary>
			/// <param name="count">How much to adjust the position by</param>
			/// <param name="from">Where to add "count".</param>
			/// <returns>The old file position.</returns>
			uoff_t SeekTo(off_t count, SeekPlace from = SEEKPLACE_BEG);

			/// <summary>
			/// Invalidates a file handle, allowing it to be used for another file.
			/// </summary>
			/// <returns>Whether the file could be closed (true) or not (false). If it fails, use GetLastError for an error code.</returns>
			bool Close();

			~FileHandle() 
			{
				if (m_flags & FLAGS_CLOSED || !m_node) 
					return;
				Close();
			}

		private:
			bool __TestEof(uoff_t pos) const;
			void* m_pathNode = nullptr; // The node that Open finds.
			void* m_node = nullptr; // The node that m_pathNode links to if it's a symlink. If m_pathNode is not a symlink, this is the same as m_pathNode.
			void* m_nodeInFileHandlesReferencing = nullptr;
			uoff_t m_currentFilePos = 0;
			uint32_t m_flags = FLAGS_CLOSED;
		};
	}
}