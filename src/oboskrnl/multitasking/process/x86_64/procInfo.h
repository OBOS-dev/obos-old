/*
	oboskrnl/process/x86_64/procInfo.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

namespace obos
{
	namespace process
	{
		struct procContextInfo
		{
			void* cr3;
		};
	}
}