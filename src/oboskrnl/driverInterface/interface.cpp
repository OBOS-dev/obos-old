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
#include <multitasking/cpu_local.h>

#include <liballoc/liballoc.h>

#include <multitasking/process/process.h>

#define getCPULocal() ((thread::cpu_local*)thread::getCurrentCpuLocalPtr())

namespace obos
{
	namespace driverInterface
	{
		// class DriverConnection

		bool DriverConnection::SendDataOnBuffer(const void* data, size_t size, con_buffer* buffer, bool spinOnLock)
		{
			if (!buffer)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!buffer->inUse.Lock(0, spinOnLock))
				return false;

			if (!buffer->buf)
			{
				// Allocate the buffer.
				buffer->buf = (byte*)kcalloc(size + 1, sizeof(byte));
				buffer->szBuf = size;
				if(data)
					utils::memcpy(buffer->buf, data, size);
				
				buffer->inUse.Unlock();
				return true;
			}
			buffer->buf = (byte*)krealloc(buffer->buf, buffer->szBuf + size + 1);
			if(data)
				utils::memcpy(buffer->buf + buffer->szBuf, data, size);
			else
				utils::memzero(buffer->buf + buffer->szBuf, size);
			buffer->szBuf += size;
			buffer->inUse.Unlock();
			return true;
		}
		bool DriverConnection::RecvDataOnBuffer(void* data, size_t size, con_buffer* buffer, bool peek, bool spinOnBuffer, uint32_t ticksToWait, bool spinOnLock)
		{
			if (!buffer)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!size)
				return true;

			if (buffer->szBuf < size && spinOnBuffer)
			{
				buffer->amountExcepted = buffer->szBuf + size;

				volatile thread::Thread*& currentThread = getCPULocal()->currentThread;
				currentThread->blockCallback.callback = [](thread::Thread* thread, void* _buff)->bool
					{
						con_buffer* buff = (con_buffer*)_buff;
						return buff->wake || (buff->szBuf >= buff->amountExcepted) || (thread::g_timerTicks > thread->wakeUpTime);
					};
				currentThread->blockCallback.userdata = buffer;
				currentThread->wakeUpTime = ticksToWait;
				currentThread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_BLOCKED;
				thread::callScheduler();
			}
			if (buffer->szBuf < size)
			{
				SetLastError(OBOS_ERROR_TIMEOUT);
				return false;
			}

			if (!buffer->inUse.Lock(0, spinOnLock))
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
					goto finish;
				}
				byte* newBuf = (byte*)kcalloc(buffer->szBuf - size, sizeof(byte));
				utils::memcpy(newBuf, buffer->buf + size, buffer->szBuf - size);
				kfree(buffer->buf);
				buffer->szBuf -= size;
				buffer->buf = newBuf;
				
			}

			finish:
			buffer->inUse.Unlock();
			return true;
		}
		
		// class DriverConnectionBase

		bool DriverConnectionBase::SendData(const void* data, size_t size, bool spinOnLock)
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
		bool DriverConnectionBase::RecvData(void* data, size_t size, uint32_t ticksToWait, bool peek, bool spinOnBuffer, bool spinOnLock)
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
				SetLastError(m_rawCon ? OBOS_ERROR_CONNECTION_CLOSED : OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			// Threads can still read from the connections, but not send.
			m_rawCon->buf1.wake = true;
			m_rawCon->buf2.wake = true;
			if (!(--m_rawCon->references))
			{
				if(m_rawCon->buf1.buf)
					kfree(m_rawCon->buf1.buf);
				if(m_rawCon->buf2.buf)
					kfree(m_rawCon->buf2.buf);
				m_rawCon->connectionOpen = false;
				driverIdentity* identity = (driverIdentity*)m_rawCon->_driverIdentity;
				DriverConnectionNode* node = identity->connections.head;
				while (node)
				{
					if (node->val == m_rawCon)
						break;

					node = node->next;
				}
				if(node->prev)
					node->prev->next = node->next;
				if (node->next)
					node->next->prev = node->prev;
				if (node == identity->connections.head)
					identity->connections.head = node->next;
				if (node == identity->connections.tail)
					identity->connections.tail = node->prev;
				identity->connections.size--;
				delete node;
				delete m_rawCon;
			}
			m_rawCon = nullptr;
			m_didWeCloseConnection = true;
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
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
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
				volatile thread::Thread*& currentThread = getCPULocal()->currentThread;
				currentThread->wakeUpTime = thread::g_timerTicks + timeoutTicks;
				currentThread->blockCallback.callback = [](thread::Thread* thr, void* _data)->bool
					{
						driverIdentity* identity = (driverIdentity*)_data;
						return identity->listening || (thread::g_timerTicks >= thr->wakeUpTime);
					};
				currentThread->blockCallback.userdata = identity;
				currentThread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_BLOCKED;
				thread::callScheduler();
			}
			if (!identity->listening)
			{
				SetLastError(OBOS_ERROR_TIMEOUT);
				return false;
			}

			m_rawCon = new DriverConnection;
			m_rawCon->connectionOpen = true;
			m_rawCon->references++;
			m_rawCon->_driverIdentity = identity;

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
		
		bool DriverClient::SendData(const void* data, size_t size, bool spinOnLock)
		{
			if (!m_rawCon || !m_rawCon->connectionOpen)
			{
				SetLastError(m_rawCon ? OBOS_ERROR_CONNECTION_CLOSED : OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			return m_rawCon->SendDataOnBuffer(data,size, &m_rawCon->buf2, spinOnLock);
		}
		bool DriverClient::RecvData(void* data, size_t size, uint32_t ticksToWait, bool peek, bool spinOnBuffer, bool spinOnLock)
		{
			if (!m_rawCon)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			return m_rawCon->RecvDataOnBuffer(data, size, &m_rawCon->buf1, peek, spinOnBuffer, ticksToWait, spinOnLock);
		}

		// class DriverServer

		bool DriverServer::Listen(uint64_t timeoutTicks)
		{
			if (m_rawCon)
			{
				SetLastError(OBOS_ERROR_ALREADY_EXISTS);
				return false;
			}
			volatile thread::Thread*& currentThread = getCPULocal()->currentThread;
			driverIdentity* identity = (driverIdentity*)((process::Process*)currentThread->owner)->_driverIdentity;
			identity->listening = true;
			if (!identity)
			{
				SetLastError(OBOS_ERROR_NOT_A_DRIVER);
				return false;
			}
			uintptr_t data[2] = { (uintptr_t)this, identity->connections.size };
			currentThread->wakeUpTime = timeoutTicks ? thread::g_timerTicks + timeoutTicks : 0xffffffffffffffff;
			currentThread->blockCallback.callback = ListenCallback;
			currentThread->blockCallback.userdata = data;
			currentThread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_BLOCKED;
			thread::callScheduler();
			if (!m_rawCon)
				return false;
			m_rawCon->references++;
			return true;
		}

		bool DriverServer::SendData(const void* data, size_t size, bool spinOnLock)
		{
			if (!m_rawCon || !m_rawCon->connectionOpen)
			{
				SetLastError(m_rawCon ? OBOS_ERROR_CONNECTION_CLOSED : OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			return m_rawCon->SendDataOnBuffer(data,size, &m_rawCon->buf1, spinOnLock);
		}
		bool DriverServer::RecvData(void* data, size_t size, uint32_t ticksToWait, bool peek, bool spinOnBuffer, bool spinOnLock)
		{
			if (!m_rawCon)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			return m_rawCon->RecvDataOnBuffer(data, size, &m_rawCon->buf2, peek, spinOnBuffer, ticksToWait, spinOnLock);
		}
		
		bool DriverServer::CloseConnection() { return DriverConnectionBase::CloseConnection(); }
		bool DriverClient::CloseConnection() { return DriverConnectionBase::CloseConnection(); }

		bool DriverServer::ListenCallback(thread::Thread* thr, void* userdata)
		{
			if (thread::g_timerTicks > thr->wakeUpTime)
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
			if (identity->connections.size > previousCount)
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
		void SyscallFreeDriverServer(DriverServer** obj)
		{
			delete *obj;
		}

		bool DummyClass1::Listen(uint64_t)
		{
			return false;
		}
		bool DummyClass1::IsConnectionClosed() { return true; }

		bool DummyClass2::OpenConnection(uint32_t, uint64_t)
		{
			return false;
		}
		bool DummyClass2::IsConnectionClosed() { return true; }
}
}