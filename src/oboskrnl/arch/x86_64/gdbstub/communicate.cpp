/*790
	oboskrnl/arch/x86_64/gdbstub/communicate.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <new>

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>
#include <vector.h>

#include <x86_64-utils/asm.h>

#include <arch/x86_64/gdbstub/communicate.h>

#include <multitasking/locks/mutex.h>

#include <liballoc/liballoc.h>

namespace obos
{
	namespace gdbstub
	{
		constexpr uint16_t COM1 = 0x3f8;
		locks::Mutex g_com1Lock;
		void DefaultLockConnection()
		{
			g_com1Lock.Lock(0, true);
		}
		void DefaultUnlockConnection()
		{
			g_com1Lock.Unlock();
		}
		void DefaultSendByteOnRawConnection(byte ch)
		{
			// Wait for com1.lineStatusRegister->transmitterHoldingRegisterEmpty
			while (!(inb(COM1 + 5) & (1 << 5)));
			outb(COM1, ch);
			outb(0xE9, ch);
		}
		byte DefaultRecvByteOnRawConnection()
		{
			// Wait for com1.lineStatusRegister->dataReady
			while (!(inb(COM1 + 5) & (1 << 0)));
			return inb(COM1);
		}
		bool DefaultByteInConnBuffer()
		{
			return inb(COM1 + 5) & (1 << 0);
		}
		uintptr_t hex2bin(const char* str, unsigned size)
		{
			uintptr_t ret = 0;
			str += *str == '\n';
			//unsigned size = utils::strlen(str);
			for (int i = size - 1, j = 0; i > -1; i--, j++)
			{
				char c = str[i];
				uintptr_t digit = 0;
				switch (c)
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					digit = c - '0';
					break;
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
					digit = (c - 'A') + 10;
					break;
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					digit = (c - 'a') + 10;
					break;
				default:
					break;
				}
				/*if (!j)
				{
					ret = digit;
					continue;
				}*/
				ret |= digit << (j * 4);
			}
			return ret;
		}
		byte mod256(const char* data, size_t len)
		{
			byte checksum = 0;
			for (size_t i = 0; i < len; i++)
				checksum += data[i];
			return checksum;
		}
		size_t countChInStr(const char* str, char ch, size_t len)
		{
			size_t i = 0, ret = 0;
			for (; i < len; ret += (str[i++] == ch));
			return ret;
		}
		bool Connection::SendRawPacket(const Packet& packet)
		{
			if (!packet.data && packet.len > 0)
				return false;
			// Form the packet.
			size_t len = packet.len + 5;
			if (packet.data)
			{
				len += countChInStr(packet.data, '#', packet.len) * 2;
				len += countChInStr(packet.data, '$', packet.len) * 2;
				len += countChInStr(packet.data, '}', packet.len) * 2;
			}
			char* data = nullptr;
			if (!packet.data)
			{
				data = (char*)"$#00";
				len = 4;
				goto send;
			}
			data = new char[len];
			data[0] = '$';
			//utils::memcpy(data + 1, packet.data, packet.len);
			{
				char* iter = data + 1;
				for (size_t i = 0; i < packet.len; i++)
				{
					char ch = packet.data[i];
					if (packet.data[i] == '#' || packet.data[i] == '$' || packet.data[i] == '}')
					{
						// Escape the character.
						*(iter++) = 0x7d;
						ch ^= 0x20;
					}
					*(iter++) = ch;
				}
			}
			data[packet.len + 1] = '#';
			data[packet.len + 2] = '\0';
			data[packet.len + 3] = '\0';
			data[packet.len + 4] = '\0';
			logger::sprintf(data + packet.len + 2, "%e%x", 2, mod256(data + 1, len - 5));
			// Send the packet.
			send:
			m_lockConnection();
			size_t tries = 0;
		retry:
			for (size_t i = 0; i < len; m_sendByteOnRawConnection(data[i++]));
			if (m_sendingACK)
			{
				if (m_recvByteOnRawConnection() == '-')
				{
					if (++tries == 5)
					{
						delete[] data;
						return false;
					}
					goto retry;
				}
			}
			m_unlockConnection();
			delete[] data;
			return true;
		}
		bool Connection::RecvRawPacket(Packet& packet)
		{
			size_t tries = 0;
			Vector<byte> rawPacket;
			m_lockConnection();
			begin:
			char ch = m_recvByteOnRawConnection();
			while (ch != '$')
				ch = m_recvByteOnRawConnection();
			bool isEscapeSequence = false;
			for (ch = m_recvByteOnRawConnection(); ch != '#'; ch = m_recvByteOnRawConnection())
			{
				if(ch != 0x7d && !isEscapeSequence)
				{
					rawPacket.push_back(ch);
					continue;
				}
				else
				{
					isEscapeSequence = true;
					continue;
				}
				if (isEscapeSequence)
				{
					rawPacket.push_back(ch ^ 0x20);
					isEscapeSequence = false;
				}

			}
			char _receivedChecksum[2] = {};
			_receivedChecksum[0] = m_recvByteOnRawConnection();
			_receivedChecksum[1] = m_recvByteOnRawConnection();
			byte receivedChecksum = hex2bin(_receivedChecksum, 2);
			byte ourChecksum = mod256((char*)&rawPacket[0], rawPacket.length());
			if (receivedChecksum != ourChecksum && m_sendingACK)
			{
				if (++tries == 5)
				{
					m_unlockConnection();
					rawPacket.clear();
					return false;
				}
				rawPacket.clear();
				m_sendByteOnRawConnection('-');
				goto begin;
			}
			if (m_sendingACK)
				m_sendByteOnRawConnection('+');
			m_unlockConnection();
			packet.data = (char*)utils::memcpy(new char[rawPacket.length() + 1], &rawPacket[0], rawPacket.length());
			packet.len = rawPacket.length();
			return true;
		}
		void InitDefaultConnection()
		{
			new (&g_com1Lock) locks::Mutex{ false };
			outb(COM1 + 1, 0x00);
			outb(COM1 + 3, 0x80);
			outb(COM1 + 0, 0x01);
			outb(COM1 + 1, 0x00);
			outb(COM1 + 3, 0x03);
			outb(COM1 + 2, 0xC7);
			outb(COM1 + 4, 0x0B);
			outb(COM1 + 4, 0x1E);
			outb(COM1 + 0, 0xAE);

			if (inb(COM1 + 0) != 0xAE)
				logger::panic("Could not initialize the gdb stub connection.\n");
			
			outb(COM1 + 4, 0x0F);
		}
	}
}