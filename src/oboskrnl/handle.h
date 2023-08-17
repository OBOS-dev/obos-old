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
	protected:
		void* m_value = nullptr;
		Handle* m_origin = nullptr;
		// If m_origin != this this field is unused.
		SIZE_T m_references = 0;
	};
}