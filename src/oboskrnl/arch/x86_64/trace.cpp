/*
	oboskrnl/arch/x86_64/trace.cpp
	
	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>

namespace obos
{
	namespace logger
	{
		struct stack_frame
		{
			stack_frame* down;
			void* rip;
		};
		void stackTrace()
		{
			volatile stack_frame* frame = (volatile stack_frame*)__builtin_frame_address(0);
			printf("Stack trace:\n");
			int nFrames = -1;
			for (; frame->down; frame = frame->down, nFrames++);
			frame = (volatile stack_frame*)__builtin_frame_address(0);
			for (int i = nFrames; i != -1; i--, frame = frame->down)
				printf("\t%p: %p\n", i, frame->rip);
		}
	}
}