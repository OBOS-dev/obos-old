/*
	mutexHandle.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <multitasking/mutex/mutexHandle.h>
#include <multitasking/mutex/mutex.h>

#include <multitasking/multitasking.h>

#include <process/process.h>

namespace obos
{
	namespace multitasking
	{
		MutexHandle::MutexHandle()
		{
			m_value = new Mutex();
			m_origin = this;
			m_references = 1;
			if (process::g_nextProcessId)
				list_rpush(g_currentThread->owner->abstractHandles, list_node_new(this));
		}

		Handle* MutexHandle::duplicate()
		{
			MutexHandle* newHandle = new MutexHandle{};
			newHandle->m_origin = m_origin;
			delete reinterpret_cast<MutexHandle*>(newHandle->m_value);
			newHandle->m_value = m_value;
			newHandle->m_references = 0;
			m_references++;
			return newHandle;
		}
		int MutexHandle::closeHandle()
		{
			if(!(--m_origin->getReferences()))
				delete (MutexHandle*)m_value;
			if (process::g_nextProcessId)
				list_remove(g_currentThread->owner->abstractHandles, list_find(g_currentThread->owner->abstractHandles, this));
			return m_origin->getReferences();
		}

		bool MutexHandle::Lock(bool waitIfLocked)
		{
			if (!m_value)
				return false;
			Mutex* mutex = (Mutex*)m_value;
			mutex->Lock(waitIfLocked);
			return mutex->m_locked;
			
		}
		void MutexHandle::Unlock()
		{
			if (!m_value)
				return;
			Mutex* mutex = (Mutex*)m_value;
			mutex->Unlock();
		}

		bool MutexHandle::IsLocked()
		{
			Mutex* mutex = (Mutex*)m_value;
			return mutex->m_locked;
		}

		MutexHandle::~MutexHandle()
		{
			closeHandle();
		}
	}
}