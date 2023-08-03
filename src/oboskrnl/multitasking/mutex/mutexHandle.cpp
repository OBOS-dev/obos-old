/*
	mutexHandle.cpp

	Copyright (c) 2023 Omar Berrow
*/
#include <multitasking/mutex/mutexHandle.h>
#include <multitasking/mutex/mutex.h>

#include <memory_manager/liballoc/liballoc.h>

namespace obos
{
	namespace multitasking
	{
		MutexHandle::MutexHandle()
		{
			m_value = new Mutex();
			m_origin = this;
			m_references = 1;
		}

		Handle* MutexHandle::duplicate()
		{
			MutexHandle* newHandle = (MutexHandle*)kcalloc(1, sizeof(MutexHandle));
			newHandle->m_origin = m_origin;
			m_references++;
			return newHandle;
		}
		int MutexHandle::closeHandle()
		{
			if(m_origin->getReferences()--)
				delete (MutexHandle*)m_value;
			return m_origin->getReferences();
		}

		void MutexHandle::Lock(bool waitIfLocked)
		{
			if (!m_value)
				return;
			Mutex* mutex = (Mutex*)m_value;
			mutex->Lock(waitIfLocked);
		}
		void MutexHandle::Unlock()
		{
			if (!m_value)
				return;
			Mutex* mutex = (Mutex*)m_value;
			mutex->Unlock();
		}

		MutexHandle::~MutexHandle()
		{
			closeHandle();
		}
	}
}