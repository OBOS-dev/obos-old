/*
	driverInterface/interface.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <memory_manipulation.h>

#include <driverInterface/interface.h>
#include <driverInterface/struct.h>

#include <multitasking/scheduler.h>

#include <liballoc/liballoc.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace driverInterface
	{
		// class DriverConnection

		bool DriverConnection::SendDataOnBuffer(const byte* data, size_t size, con_buffer* buffer, bool spinOnLock)
		{
			if (!buffer)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETERS);
				return false;
			}
			if (!AttemptLock(buffer, spinOnLock))
				return false;

			if (!buffer->buf)
			{
				// Allocate the buffer.
				buffer->buf = (byte*)kcalloc(size + 1, sizeof(byte));
				buffer->szBuf = size;
				utils::memcpy(buffer->buf, data, size);
				
				AttemptUnlock(buffer);
				return true;
			}
			buffer->buf = (byte*)krealloc(buffer->buf, buffer->szBuf + size + 1);
			utils::memcpy(buffer->buf + buffer->szBuf, data, size);
			buffer->szBuf += size;

			AttemptUnlock(buffer);
			return true;
		}
		bool DriverConnection::RecvDataOnBuffer(byte* data, size_t size, con_buffer* buffer, bool isConnectionClosed, bool peek, bool spinOnBuffer, uint32_t ticksToWait, bool spinOnLock)
		{
			if (!buffer)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETERS);
				return false;
			}
			
			if (buffer->szBuf < size && spinOnBuffer)
			{
				buffer->amountExcepted = buffer->szBuf + size;

				thread::g_currentThread->blockCallback.callback = [](thread::Thread* thread, void* _buff)->bool
					{
						con_buffer* buff = (con_buffer*)_buff;
						return buff->wake || (buff->szBuf >= buff->amountExcepted) || (thread::g_timerTicks > thread->wakeUpTime);
					};
				thread::g_currentThread->blockCallback.userdata = buffer;
				thread::g_currentThread->wakeUpTime = ticksToWait;
				thread::g_currentThread->status |= thread::THREAD_STATUS_BLOCKED;
				thread::callScheduler();
			}
			if (buffer->szBuf < size)
			{
				SetLastError(OBOS_ERROR_TIMEOUT);
				return false;
			}

			if (!AttemptLock(buffer, spinOnLock))
				return false;

			if(data)
				utils::memcpy(data, buffer->buf, size);

			if (!peek)
			{
				if (!(buffer->szBuf - size))
				{
					buffer->szBuf = 0;
					kfree(buffer->buf);
					buffer->buf = nullptr;
					if (isConnectionClosed)
					{

					}
					goto finish;
				}
				byte* newBuf = (byte*)kcalloc(buffer->szBuf - size, sizeof(byte));
				utils::memcpy(newBuf, buffer->buf + size, buffer->szBuf - size);
				kfree(buffer->buf);
				buffer->szBuf -= size;
				buffer->buf = newBuf;
				
			}

			finish:
			AttemptUnlock(buffer);
			return true;
		}
		bool DriverConnection::AttemptLock(con_buffer* buffer, bool spinOnLock)
		{
			if (buffer->inUse && spinOnLock)
			{
				thread::g_currentThread->blockCallback.callback = [](thread::Thread*, void* _buff)->bool
					{
						con_buffer* buff = (con_buffer*)_buff;
						return !buff->inUse || buff->wake;
					};
				thread::g_currentThread->blockCallback.userdata = buffer;
				thread::g_currentThread->status |= thread::THREAD_STATUS_BLOCKED;
				thread::callScheduler();
			}
			if (buffer->inUse)
			{
				SetLastError(OBOS_ERROR_MUTEX_LOCKED);
				return false;
			}
			buffer->inUse = true;
			return true;
		}
		bool DriverConnection::AttemptUnlock(con_buffer* buffer)
		{
			buffer->inUse = false;
			return true;
		}

		// class DriverConnectionBase

		bool DriverConnectionBase::SendData(const byte* data, size_t size, bool spinOnLock)
		{
			(void)data;
			(void)size;
			(void)spinOnLock;
			byte* _data = (byte*)data;
			_data[0] = _data[0];
			_data[0] = _data[0] + 1;
			_data[0] = _data[0] - 1;
			return false;
		}
		bool DriverConnectionBase::RecvData(byte* data, size_t size, bool peek, bool spinOnBuffer, uint32_t ticksToWait, bool spinOnLock)
		{
			(void)size;
			(void)peek;
			(void)spinOnBuffer;
			(void)ticksToWait;
			(void)spinOnLock;
			byte* _data = (byte*)data;
			_data[0] = _data[0];
			_data[0] = _data[0] + 1;
			_data[0] = _data[0] - 1;
			return false;
		}

		bool DriverConnectionBase::CloseConnection()
		{
			if (!m_rawCon || !m_rawCon->connectionOpen)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			// Threads can still read from the connections, but not send.
			m_rawCon->connectionOpen = false;
			thread::callScheduler();
			return true;
		}

		// class DriverClient

		bool DriverClient::OpenConnection(uint32_t pid, uint64_t timeoutTicks)
		{
			if (m_rawCon)
			{
				SetLastError(OBOS_ERROR_ALREADY_EXISTS);
				return false;
			}
			if (pid >= process::g_processes.size)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETERS);
				return false;
			}
			bool direction = pid < (process::g_processes.size / 2);
			process::Process* proc = direction ? process::g_processes.head : process::g_processes.tail;
			for (; proc;)
			{
				if (proc->pid == pid)
					break;

				if (direction)
					proc = proc->next;
				else
					proc = proc->prev;
			}
			if (!proc->_driverIdentity)
			{
				SetLastError(OBOS_ERROR_NOT_A_DRIVER);
				return false;
			}
			driverIdentity* identity = (driverIdentity*)proc->_driverIdentity;

			if (!identity->listening && timeoutTicks)
			{
				thread::g_currentThread->wakeUpTime = thread::g_timerTicks + timeoutTicks;
				thread::g_currentThread->blockCallback.callback = [](thread::Thread* thr, void* _data)->bool
					{
						driverIdentity* identity = (driverIdentity*)_data;
						return identity->listening || (thread::g_timerTicks >= thr->wakeUpTime);
					};
				thread::g_currentThread->blockCallback.userdata = identity;
				thread::g_currentThread->status |= thread::THREAD_STATUS_BLOCKED;
				thread::callScheduler();
			}
			if (!identity->listening)
			{
				SetLastError(OBOS_ERROR_TIMEOUT);
				return false;
			}

			m_rawCon = new DriverConnection;
			m_rawCon->connectionOpen = true;

			DriverConnectionList& list = identity->connections;
			DriverConnectionNode* node = new DriverConnectionNode;
			node->val = m_rawCon;
			if (list.tail)
			{
				list.tail->next = node;
				node->prev = list.tail;
			}
			if (!list.head)
				list.head = node;
			list.tail = node;
			list.size++;
			
			return true;
		}
		
		bool DriverClient::SendData(const byte* data, size_t size, bool spinOnLock)
		{
			if (!m_rawCon || !m_rawCon->connectionOpen)
			{
				SetLastError(m_rawCon ? OBOS_ERROR_CONNECTION_CLOSED : OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			return m_rawCon->SendDataOnBuffer(data,size, &m_rawCon->buf2, spinOnLock);
		}
		bool DriverClient::RecvData(byte* data, size_t size, bool peek, bool spinOnBuffer, uint32_t ticksToWait, bool spinOnLock)
		{
			if (!m_rawCon)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			return m_rawCon->RecvDataOnBuffer(data, size, &m_rawCon->buf1, peek, spinOnBuffer, ticksToWait, spinOnLock);
		}

		// class DriverServer

		bool DriverServer::Listen(uint32_t timeoutTicks)
		{
			if (m_rawCon)
			{
				SetLastError(OBOS_ERROR_ALREADY_EXISTS);
				return false;
			}
			driverIdentity* identity = (driverIdentity*)((process::Process*)thread::g_currentThread->owner)->_driverIdentity;
			identity->listening = true;
			if (!identity)
			{
				SetLastError(OBOS_ERROR_NOT_A_DRIVER);
				return false;
			}
			uintptr_t data[2] = { (uintptr_t)this, identity->connections.size };
			thread::g_currentThread->wakeUpTime = thread::g_timerTicks + timeoutTicks;
			thread::g_currentThread->blockCallback.callback = ListenCallback;
			thread::g_currentThread->blockCallback.userdata = data;
			thread::g_currentThread->status |= thread::THREAD_STATUS_BLOCKED;
			thread::callScheduler();
			if (!this->m_rawCon)
				return false;
			return true;
		}

		bool DriverServer::SendData(const byte* data, size_t size, bool spinOnLock)
		{
			if (!m_rawCon || !m_rawCon->connectionOpen)
			{
				SetLastError(m_rawCon ? OBOS_ERROR_CONNECTION_CLOSED : OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			return m_rawCon->SendDataOnBuffer(data,size, &m_rawCon->buf1, spinOnLock);
		}
		bool DriverServer::RecvData(byte* data, size_t size, bool peek, bool spinOnBuffer, uint32_t ticksToWait, bool spinOnLock)
		{
			if (!m_rawCon)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			return m_rawCon->RecvDataOnBuffer(data, size, &m_rawCon->buf2, peek, spinOnBuffer, ticksToWait, spinOnLock);
		}

		bool DriverServer::ListenCallback(thread::Thread* thr, void* userdata)
		{
			if (thread::g_timerTicks >= thr->wakeUpTime)
			{
				thr->lastError = OBOS_ERROR_TIMEOUT;
				return true;
			}
			driverIdentity* identity = (driverIdentity*)((process::Process*)thr->owner)->_driverIdentity;
			uintptr_t* data = (uintptr_t*)userdata;
			DriverServer* _this = (DriverServer*)data[0];
			uintptr_t previousCount = data[1];
			if ((previousCount + 1) == identity->connections.size)
			{
				DriverConnectionList& list = identity->connections;
				_this->m_rawCon = list.tail->val;
				return true;
			}
			if (previousCount > identity->connections.size)
			{
				// Get the element at "previousElement"
				DriverConnectionList& list = identity->connections;
				bool direction = previousCount < (list.size / 2);
				DriverConnectionNode* node = direction ? list.head : list.tail;
				for (size_t i = 0; node; )
				{
					if (i == previousCount)
						break;

					if (direction)
						node = node->next;
					else
						node = node->prev;
				}
				_this->m_rawCon = node->val;
				return true;
			}
			return false;
		}
		
		DriverServer* SyscallAllocDriverServer()
		{
			return new DriverServer;
		}
		void SyscallFreeDriverServer(DriverServer* obj)
		{
			delete obj;
		}

		bool DummyClass1::Listen(uint32_t)
		{
			return false;
		}
		bool DummyClass2::OpenConnection(uint32_t, uint64_t)
		{
			return false;
		}
}
}