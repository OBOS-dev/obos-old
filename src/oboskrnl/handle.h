/*
	handle.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

namespace obos
{
	class Handle
	{
	public:
		enum class handleType
		{
			abstract,
			thread,
		};

	public:
		Handle();

		constexpr static handleType getType()
		{
			return handleType::abstract;
		}
		virtual Handle* duplicate() = 0;
		virtual int closeHandle() = 0;

		virtual ~Handle() {}
	protected:
		[[maybe_unused]] void* m_value;
		Handle* m_origin;
		// If m_origin != this this field is unused.
		[[maybe_unused]] SIZE_T m_references = 0;
	};
}