/*
	oboskrnl/vfs/fileHandle.h

	Copyright (c) 2023 Omar Berrow
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

			bool Write() { return false; } // Not implemented.

			bool Eof() const;
			uoff_t GetPos() { return m_currentFilePos; }
			uint32_t GetFlags() { return m_flags; }
			size_t GetFileSize();
			void GetParent(char* path, size_t* sizePath);

			/// <summary>
			/// Seeks to where based on the parameters.
			/// </summary>
			/// <param name="count">How much to adjust the position by</param>
			/// <param name="from">Where to add "count".</param>
			/// <returns>The old file position.</returns>
			uoff_t SeekTo(off_t count, SeekPlace from = SEEKPLACE_BEG);

			bool Close();

			~FileHandle();

		private:
			void* m_pathNode = nullptr; // The node that Open finds.
			void* m_node = nullptr; // The node that m_pathNode links to if it's a symlink. If m_pathNode is not a symlink, this is the same as m_pathNode.
			uoff_t m_currentFilePos = 0;
			uint32_t m_flags = 0;
		};
	}
}