/*
	oboskrnl/threadAPI/thrHandle.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace thread
	{
		class ThreadHandle
		{
		public:
			ThreadHandle();
			
			bool OpenThread(uint32_t tid);
			
			bool CreateThread(uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, bool startPaused = false, bool isUsermode = false);
		private:
			void* m_obj = nullptr;
		};
	}
}