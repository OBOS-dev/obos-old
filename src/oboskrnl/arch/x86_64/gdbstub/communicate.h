/*
	oboskrnl/arch/x86_64/gdbstub/communicate.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace gdbstub
	{
		struct Packet
		{
			char* data;
			size_t len;
		};
		void DefaultLockConnection();
		void DefaultUnlockConnection();
		void DefaultSendByteOnRawConnection(byte);
		byte DefaultRecvByteOnRawConnection();
		bool DefaultByteInConnBuffer();
		void InitDefaultConnection();
		byte mod256(const char* data, size_t len);
		class Connection
		{
		public:
			Connection()
				: m_sendByteOnRawConnection{ DefaultSendByteOnRawConnection }, 
				  m_recvByteOnRawConnection{ DefaultRecvByteOnRawConnection },
				  m_lockConnection{ DefaultLockConnection },
				  m_unlockConnection{ DefaultUnlockConnection },
				  m_isByteInConnBuffer{ DefaultByteInConnBuffer }
			{}
			Connection(void(*sendByteOnRawConnection)(byte), byte(*recvByteOnRawConnection)(), void(*lockConnection)(), void(*unlockConnection)(), bool(*isByteInConnBuffer)())
				: m_sendByteOnRawConnection{ sendByteOnRawConnection }, 
				  m_recvByteOnRawConnection{ recvByteOnRawConnection },
				  m_lockConnection{ lockConnection },
				  m_unlockConnection{ unlockConnection },
				  m_isByteInConnBuffer { isByteInConnBuffer }
			{}

			bool SendRawPacket(const Packet& packet);
			bool RecvRawPacket(Packet& packet);

			bool CanReadByte() { return m_isByteInConnBuffer(); }
		private:
			void(*m_sendByteOnRawConnection)(byte);
			byte(*m_recvByteOnRawConnection)();
			void(*m_lockConnection)();
			void(*m_unlockConnection)();
			bool(*m_isByteInConnBuffer)();
		};
	}
}