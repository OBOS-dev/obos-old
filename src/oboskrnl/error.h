/*
	oboskrnl/error.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	enum obos_error
	{
		OBOS_SUCCESS,
		OBOS_ERROR_NO_SUCH_OBJECT,
		OBOS_ERROR_INVALID_PARAMETERS,

		OBOS_ERROR_HIGHEST_VALUE,
	};
	void SetLastError(uint32_t err);
	uint32_t GetLastError();
}