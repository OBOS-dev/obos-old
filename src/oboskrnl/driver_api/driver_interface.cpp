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
#include <multitasking/threadHandle.h>

namespace obos
{
	namespace driverAPI
	{
		bool DriverConnectionHandle::SendData(PBYTE buffer, SIZE_T size, bool failIfConnectionMutexLocked)
		{
			return SendDataImpl(m_outputBuffer, buffer, size, failIfConnectionMutexLocked);
		}
		bool DriverConnectionHandle::RecvData(PBYTE buffer, SIZE_T size, bool waitForData, bool peek, bool failIfConnectionMutexLocked)
		{
			return RecvDataImpl(m_inputBuffer, buffer, size, waitForData, peek, failIfConnectionMutexLocked);
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

		bool RecvDataCallback(multitasking::Thread*, PVOID _udata)
		{
			UINTPTR_T* userData = (UINTPTR_T*)_udata;
			connection_buffer* _this = (connection_buffer*)userData[0];
			SIZE_T size = userData[1];
			return _this->szBuffer >= size;
		}

		bool DriverConnectionHandle::RecvDataImpl(connection_buffer& conn_buffer, PBYTE data, SIZE_T size, bool waitForData, bool peek, bool failIfConnectionMutexLocked)
		{
			if (!m_driverIdentity)
			{
				SetLastError(OBOS_ERROR_UNOPENED_HANDLE);
				return false;
			}

			if (size > conn_buffer.szBuffer)
			{
				if(!waitForData)
				{
					SetLastError(OBOS_ERROR_BUFFER_TOO_SMALL);
					return false;
				}
				UINTPTR_T udata[2] = { (UINTPTR_T)&conn_buffer, size };
				multitasking::g_currentThread->isBlockedCallback = RecvDataCallback;
				multitasking::g_currentThread->isBlockedUserdata = udata;
				multitasking::g_currentThread->status |= (DWORD)multitasking::Thread::status_t::BLOCKED;
				_int(0x30);
			}

			multitasking::safe_lock lock{ &conn_buffer.mutex };
			if (!lock.Lock(failIfConnectionMutexLocked))
				return false;

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
				utils::memcpy(newBuffer, conn_buffer.buffer + size, conn_buffer.szBuffer - size);
				kfree(conn_buffer.buffer);
				conn_buffer.bufferPosition = newBuffer + (conn_buffer.szBuffer - size);
				conn_buffer.szBuffer -= size;
				conn_buffer.buffer = newBuffer;
			}

			return true;
		}
		
		bool OpenConnectionCallback(multitasking::Thread* _this, PVOID userData)
		{
			driverIdentification* identity = (driverIdentification*)userData;
			return identity->isListeningForConnection || (multitasking::g_timerTicks >= _this->wakeUpTime);
		}
		bool DriverClientConnectionHandle::OpenConnection(DWORD driverId, DWORD connectionTimeoutMilliseconds)
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
			if(!identity->isListeningForConnection)
			{
				multitasking::g_currentThread->isBlockedCallback = OpenConnectionCallback;
				multitasking::g_currentThread->isBlockedUserdata = identity;
				multitasking::g_currentThread->wakeUpTime = multitasking::g_timerTicks + multitasking::MillisecondsToTicks(connectionTimeoutMilliseconds);
				multitasking::g_currentThread->status |= multitasking::Thread::status_t::BLOCKED;
				_int(0x30);
				if (!identity->isListeningForConnection)
				{
					SetLastError(OBOS_ERROR_TIMEOUT);
					return false;
				}
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
		bool DriverClientConnectionHandle::RecvData(PBYTE data, SIZE_T size, bool waitForData, bool peek, bool failIfConnectionMutexLocked)
		{
			return m_driverConnection->RecvDataImpl(m_driverConnection->m_outputBuffer, data, size, waitForData, peek, failIfConnectionMutexLocked);
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
				if (!node || !ret)
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
			driverIdentity->isListeningForConnection = true;
			multitasking::g_currentThread->isBlockedCallback = ListenBlockCallback;
			multitasking::g_currentThread->isBlockedUserdata = _ret;
			multitasking::g_currentThread->status |= (DWORD)multitasking::Thread::status_t::BLOCKED;
			_int(0x30);
			return ret;
		}
}
}