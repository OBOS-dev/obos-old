/*
	oboskrnl/driver_api/driver_interface.h

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>
#include <error.h>

#include <driver_api/driver_interface.h>

#include <memory_manager/liballoc/liballoc.h>

#include <utils/memory.h>

#include <multitasking/multitasking.h>
#include <multitasking/thread.h>

namespace obos
{
	namespace driverAPI
	{
		bool DriverConnectionHandle::SendData(PBYTE buffer, SIZE_T size, bool failIfConnectionMutexLocked)
		{
			return SendDataImpl(m_outputBuffer, buffer, size, failIfConnectionMutexLocked);
		}
		bool DriverConnectionHandle::RecvData(PBYTE buffer, SIZE_T size, bool peek, bool failIfConnectionMutexLocked)
		{
			return RecvDataImpl(m_inputBuffer, buffer, size, peek, failIfConnectionMutexLocked);
		}

		bool DriverConnectionHandle::CloseConnection(bool setLastError)
		{
			if (!m_driverIdentity)
			{
				if (setLastError)
					SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			list_remove(m_driverIdentity->driverConnections, list_find(m_driverIdentity->driverConnections, this));
			m_inputBuffer.mutex.Lock();
			m_outputBuffer.mutex.Lock();
			if (m_inputBuffer.buffer)
				kfree(m_inputBuffer.buffer);
			if (m_outputBuffer.buffer)
				kfree(m_outputBuffer.buffer);
			m_outputBuffer.mutex.closeHandle();
			m_inputBuffer.mutex.closeHandle();
			m_driverIdentity = nullptr;
			return true;
		}

		DriverConnectionHandle::~DriverConnectionHandle()
		{
			CloseConnection(false);
		}

		bool DriverConnectionHandle::SendDataImpl(connection_buffer& conn_buffer, PBYTE buffer, SIZE_T size, bool failIfConnectionMutexLocked)
		{
			if (!m_driverIdentity)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			multitasking::safe_lock lock{ &conn_buffer.mutex };
			if (!lock.Lock(failIfConnectionMutexLocked))
				return false;

			if (!conn_buffer.buffer)
			{
				conn_buffer.bufferPosition = conn_buffer.buffer = (PBYTE)kcalloc(size, sizeof(BYTE));
				if (!conn_buffer.buffer)
					return false;
			}
			else
			{
				conn_buffer.buffer = (PBYTE)krealloc(conn_buffer.buffer, conn_buffer.szBuffer + size);
				conn_buffer.bufferPosition = conn_buffer.buffer + conn_buffer.szBuffer;
				if (!conn_buffer.buffer)
					return false;
			}

			utils::memcpy(conn_buffer.bufferPosition, buffer, size);
			conn_buffer.szBuffer += size;

			return true;
		}

		bool DriverConnectionHandle::RecvDataImpl(connection_buffer& conn_buffer, PBYTE data, SIZE_T size, bool peek, bool failIfConnectionMutexLocked)
		{
			if (!m_driverIdentity)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			multitasking::safe_lock lock{ &conn_buffer.mutex };
			if (!lock.Lock(failIfConnectionMutexLocked))
				return false;

			if (size > conn_buffer.szBuffer)
			{
				SetLastError(OBOS_ERROR_BUFFER_TOO_SMALL);
				return false;
			}

			if(data)
				utils::memcpy(data, conn_buffer.buffer, size);

			if ((conn_buffer.szBuffer - size) == 0)
			{
				conn_buffer.bufferPosition = nullptr;
				conn_buffer.szBuffer = 0;
				kfree(conn_buffer.buffer);
				conn_buffer.buffer = nullptr;
				return true;
			}

			if (!peek)
			{
				PBYTE newBuffer = (PBYTE)kcalloc(conn_buffer.szBuffer - size, sizeof(BYTE));
				if (!newBuffer)
					return false;
				utils::memcpy(newBuffer, conn_buffer.buffer + size, size);
				conn_buffer.bufferPosition = newBuffer + (conn_buffer.szBuffer - size);
				conn_buffer.szBuffer -= size;
				conn_buffer.buffer = newBuffer;
				kfree(conn_buffer.buffer);
			}

			return true;
		}
		
		bool DriverClientConnectionHandle::OpenConnection(DWORD driverId)
		{
			if (g_registeredDriversCapacity < driverId)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return false;
			}
			driverIdentification* identity = g_registeredDrivers[driverId];
			if (!identity)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return false;
			}
			m_driverConnection = new DriverConnectionHandle{};
			m_driverConnection->m_driverIdentity = identity;
			list_rpush(identity->driverConnections, list_node_new(m_driverConnection));
			return true;
		}
		bool DriverClientConnectionHandle::CloseConnection(bool setLastError)
		{
			if (!m_driverConnection)
			{
				if (setLastError)
					SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}
			return m_driverConnection->CloseConnection(setLastError);
		}

		bool DriverClientConnectionHandle::SendData(PBYTE buffer, SIZE_T size, bool failIfConnectionMutexLocked)
		{
			return m_driverConnection->SendDataImpl(m_driverConnection->m_inputBuffer, buffer, size, failIfConnectionMutexLocked);
		}
		bool DriverClientConnectionHandle::RecvData(PBYTE data, SIZE_T size, bool peek, bool failIfConnectionMutexLocked)
		{
			return m_driverConnection->RecvDataImpl(m_driverConnection->m_outputBuffer, data, size, peek, failIfConnectionMutexLocked);
		}

		DriverClientConnectionHandle::~DriverClientConnectionHandle()
		{
			CloseConnection(false);
		}

		static bool ListenBlockCallback(multitasking::Thread* _this, PVOID userdata)
		{
			UINTPTR_T* _ret = reinterpret_cast<UINTPTR_T*>(userdata);
			driverIdentification* driverIdentity = (driverIdentification*)_this->owner->driverIdentity;
			if (!driverIdentity)
			{
				_this->lastError = OBOS_ERROR_PREMATURE_PROCESS_EXIT;
				return true; // Resume the thread.
			}
			if (driverIdentity->driverConnections->len > _ret[1])
			{
				DriverConnectionHandle** ret = (DriverConnectionHandle**)_ret[0];
				list_node_t* node = list_at(driverIdentity->driverConnections, _ret[1]);
				if (!node)
					return false;
				*ret = (DriverConnectionHandle*)(node->val);
				return true;
			}
			return false;
		}

		DriverConnectionHandle* Listen()
		{
			if (!multitasking::g_currentThread->owner->driverIdentity)
			{
				SetLastError(OBOS_ERROR_NOT_A_DRIVER);
				return nullptr;
			}
			driverIdentification* driverIdentity = (driverIdentification*)multitasking::g_currentThread->owner->driverIdentity;
			DriverConnectionHandle* ret = nullptr;
			UINTPTR_T _ret[2] = { (UINTPTR_T)&ret, driverIdentity->driverConnections->len };
			multitasking::g_currentThread->isBlockedCallback = ListenBlockCallback;
			multitasking::g_currentThread->isBlockedUserdata = _ret;
			multitasking::g_currentThread->status |= (DWORD)multitasking::Thread::status_t::BLOCKED;
			_int(0x30);
			return ret;
		}
}
}