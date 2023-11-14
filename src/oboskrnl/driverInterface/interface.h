/*
	driverInterface/interface.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/thread.h>

#include <multitasking/locks/mutex.h>

namespace obos
{
	namespace driverInterface
	{
		struct con_buffer
		{
			byte* buf;
			size_t szBuf;
			size_t amountExcepted; // For RecvDataOnBuffer
			bool wake;
			locks::Mutex inUse;
		};
		class DriverConnection
		{
		public:
			DriverConnection() = default;

			bool SendDataOnBuffer(const void* data, size_t size, con_buffer* buffer, bool spinOnLock = true);
			bool RecvDataOnBuffer(void* data, size_t size, con_buffer* buffer, bool peek = false, bool spinOnBuffer = true, uint32_t ticksToWait = 1000, bool spinOnLock = true);
			
			con_buffer buf1;
			con_buffer buf2;
			bool connectionOpen = false;
			size_t references;
			void* _driverIdentity = nullptr;
			friend class DriverConnectionBase;
		};
		struct DriverConnectionNode
		{
			DriverConnectionNode* next;
			DriverConnectionNode* prev;
			DriverConnection* val;
		};
		struct DriverConnectionList
		{
			DriverConnectionNode* head;
			DriverConnectionNode* tail;
			size_t size;
		};

		class DriverConnectionBase
		{
		public:
			DriverConnectionBase() = default;

#pragma GCC push_options
#pragma GCC optimize("O0")
			virtual bool SendData(const void* data, size_t size, bool spinOnLock = true);
			virtual bool RecvData(void* data, size_t size, uint32_t ticksToWait = 1000, bool peek = false, bool spinOnBuffer = true, bool spinOnLock = true);
#pragma GCC pop_options

			virtual bool IsConnectionClosed() { if (!m_rawCon) return false; return !(m_rawCon->references - !m_didWeCloseConnection) || !m_rawCon->connectionOpen; }

			virtual bool CloseConnection();
			
			virtual ~DriverConnectionBase() { CloseConnection(); }
		protected:
			DriverConnection* m_rawCon = nullptr;
			bool m_didWeCloseConnection = false;
		};
		class DriverServer : public DriverConnectionBase
		{
		public:
			// If timeoutTicks is zero, it waits indefinitely.
			virtual bool Listen(uint64_t timeoutTicks = 0);

			virtual bool SendData(const void* data, size_t size, bool spinOnLock = true) override;
			virtual bool RecvData(void* data, size_t size, uint32_t ticksToWait = 1000, bool peek = false, bool spinOnBuffer = true, bool spinOnLock = true) override;

			virtual bool CloseConnection() override;
		private:
			static bool ListenCallback(thread::Thread* _this, void* userdata);
		};
		class DriverClient : public DriverConnectionBase
		{
		public:
			virtual bool OpenConnection(uint32_t pid, uint64_t timeoutTicks);

			virtual bool SendData(const void* data, size_t size, bool spinOnLock = true) override;
			virtual bool RecvData(void* data, size_t size, uint32_t ticksToWait = 1000, bool peek = false, bool spinOnBuffer = true, bool spinOnLock = true) override;
			
			virtual bool CloseConnection() override;
		};

		class DummyClass1 final : public DriverServer
		{
		public:
			virtual bool Listen(uint64_t timeoutTicks = 0) override;
			virtual bool IsConnectionClosed() override;
		};
		class DummyClass2 final : public DriverClient
		{
		public:
			virtual bool OpenConnection(uint32_t pid, uint64_t timeoutTicks) override;
			virtual bool IsConnectionClosed() override;
		};

		// Syscall 2
		DriverServer* SyscallAllocDriverServer();
		// Syscall 3
		void SyscallFreeDriverServer(DriverServer** obj);
	}
}