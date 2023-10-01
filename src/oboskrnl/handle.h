/*
	handle.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#define OBOS_HANDLE_MAGIC_NUMBER 0x4031AA2F

namespace obos
{
	class Handle
	{
	public:
		enum class handleType
		{
			abstract,
			thread,
			mutex
		};

	public:
		Handle() = default;

		virtual handleType getType()
		{
			return handleType::abstract;
		}
		virtual Handle* duplicate() = 0;
		virtual int closeHandle() = 0;

		SIZE_T& getReferences() { return m_origin->m_references; }
		
		virtual ~Handle() {}

		DWORD magicNumber = OBOS_HANDLE_MAGIC_NUMBER;
	protected:
		void* m_value = nullptr;
		Handle* m_origin = nullptr;
		// If m_origin != this this field is unused.
		SIZE_T m_references = 0;
	};
}