/*
	oboskrnl/arch/x86_64/gdbstub/stub.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <arch/x86_64/gdbstub/stub.h>
#include <arch/x86_64/gdbstub/communicate.h>

#include <arch/x86_64/irq/irq.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <arch/x86_64/interrupt.h>

#include <x86_64-utils/asm.h>

#include <vector.h>

#include <multitasking/cpu_local.h>
#include <multitasking/thread.h>

#include <multitasking/process/process.h>

namespace obos
{
	extern uint8_t g_lapicIDs[256];
	extern uint8_t g_nCores;
	extern void exception14(interrupt_frame* frame);
	extern void defaultExceptionHandler(interrupt_frame* frame);
	extern void EarlyKPanic();
	namespace gdbstub
	{
		static void notifyGdbOfException(interrupt_frame* frame, bool send = true);
		Connection* g_gdbConnection;
		enum reg
		{
			// General Purpose Registers
			RAX,RBX,RCX,
			RDX,RSI,RDI,
			RBP,RSP, R8,
			 R9,R10,R11,
			R12,R13,R14,
			R15,RIP,RFL,
			// Segment registers
			 CS, SS, DS,
			 ES, FS, GS,
			// FPU Registers
			ST0,ST1,ST2,
			ST3,ST4,ST5,
			ST6,ST7,
			FCTR1,FSTAT,
			FTAG ,FISEG,
			FIOFF,FOSEG,
			FOOFF,  FOP,
		};
		static const char* setupRegisterResponse(interrupt_frame* frame)
		{
			constexpr const char* const format =
				"%e%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x"
			    "%s%s%s%s%s%s%s%s" 
				"%s%s%s%s%s%s%s";
			constexpr const char* const reg_empty = "xxxxxxxxxxxxxxxx";
			constexpr const char* const st_empty = "xxxxxxxxxxxxxxxxxxxx";
			constexpr const char* const fctrl_empty = "xxxx";
			constexpr const char* const seg_empty = "xxxx";
			constexpr const char* const opcode_empty = "xxxx";
			size_t len = logger::sprintf(nullptr, format, 16,
				frame->rax, frame->rsi, frame->rdi, 
				frame->rbp, frame->rsp, frame-> r8,
				frame-> r9, frame->r10, frame->r11,
				frame->r12, frame->r13, frame->r14,
				frame->r15, frame->rip, frame->rflags,
				frame->cs , frame->ds , frame->ds,
				frame->ds , frame->ds , frame->ds,
				st_empty,st_empty,st_empty,st_empty,
				st_empty,st_empty,st_empty,st_empty,
				fctrl_empty,fctrl_empty,
				seg_empty, reg_empty,
				seg_empty, reg_empty,
				opcode_empty
				);
			char* response = new char[len + 1];
			logger::sprintf(response, format, 16,
				frame->rax, frame->rsi, frame->rdi,
				frame->rbp, frame->rsp, frame->r8,
				frame->r9, frame->r10, frame->r11,
				frame->r12, frame->r13, frame->r14,
				frame->r15, frame->rip, frame->rflags,
				frame->cs, frame->ds, frame->ds,
				frame->ds, frame->ds, frame->ds,
				st_empty, st_empty, st_empty, st_empty,
				st_empty, st_empty, st_empty, st_empty,
				fctrl_empty, fctrl_empty,
				seg_empty, reg_empty,
				seg_empty, reg_empty,
				opcode_empty
			);
			return response;
		}
		const char* lastExceptionPacket = nullptr;
		size_t lastExceptionPacketLen = 0;
		// returns whether the exception can return.
		static bool HandlePacket(interrupt_frame* frame)
		{
			Packet currentPacket{};
			g_gdbConnection->RecvRawPacket(currentPacket);
			Vector<char> response;
			if (currentPacket.data)
			{
				switch(currentPacket.data[0])
				{
				case '?':
				{
					const char* _response = nullptr;
					// Notify gdb of the last exception, or the current exception if there was no last exception.
					if (!lastExceptionPacket && frame)
						notifyGdbOfException(frame, false);
					_response = lastExceptionPacket;
					for (size_t i = 0; i < lastExceptionPacketLen; i++)
							response.push_back(_response[i]);
					break;
				}
				case 'H':
				{
					char operation = currentPacket.data[1];
					const char* _response = nullptr;
					size_t _responseLen = 0;
					bool shouldFree = false;
					switch (operation)
					{
					case 'g':
					{
						// Setup the response.
						_response = setupRegisterResponse(frame);
						_responseLen = utils::strlen(_response);
						shouldFree = true;
						break;
					}
					case 'G':
					{
						// TODO: Implement this.
						break;
					}
					default:
						break;
					}
					for (size_t i = 0; i < _responseLen; i++)
							response.push_back(_response[i]);
					if (shouldFree)
						delete[] _response;
					break;
				}
				case 'k':
				{
					// Restart the system.
					// Assert the INIT# pin of all processors, until we triple fault.
					for(size_t i = 0; i < g_nCores; i++)
					{
						g_localAPICAddr->interruptCommand32_63 = g_lapicIDs[i] << (56 - 32);
						g_localAPICAddr->interruptCommand0_31 = 0xC500;
						// Wait for the IPI to finish being sent
						while (g_localAPICAddr->interruptCommand0_31 & (1 << 12))
							pause();
					}
					// It didn't work, so we call EarlyKPanic();
					EarlyKPanic();
					// That didn't work, so we just hang.
					while (1);
					break;
				}
				default:
					break;
				}
			}
			g_gdbConnection->SendRawPacket({ (char*)response.data(), response.length() });
			delete[] currentPacket.data;
			currentPacket.data = nullptr;
			currentPacket.len = 0;
			return true;
		}
		static const thread::Thread* getCurrentThread()
		{
			if (thread::GetCurrentCpuLocalPtr())
				return (thread::Thread*)thread::GetCurrentCpuLocalPtr()->currentThread;
			return nullptr;
		}
		static void notifyGdbOfException(interrupt_frame* frame, bool send)
		{
			constexpr uint8_t exceptionToSignal[] = {
				8, 0, 5, 8, 0, 4, 0, 0, // Exceptions 0-7
				0, 0,11,11,11,11,11, 0, // Exceptions 8-15
				8, 0, 0, 0, 0, 8, 0, 0, // Exceptions 16-23
			};
			uint8_t signalN = exceptionToSignal[frame->intNumber];
			constexpr const char* const format = "T%x%ethread:%x;%e%s%x:%x;%x:%x;%e%x:%x";
			uint32_t tid = 0;
			if (getCurrentThread())
				tid = getCurrentThread()->tid;
			size_t len = logger::sprintf(nullptr, format, signalN, 8, tid, 16, signalN == 0x05 ? "swbreak:;" : "", RIP, frame->rip, RSP, frame->rsp, 8, RFL, frame->rflags);
			char* packet = new char[len + 1];
			logger::sprintf(packet, format, signalN, 8, tid, 16, signalN == 0x05 ? "swbreak:;" : "", RIP, frame->rip, RSP, frame->rsp, 8, RFL, frame->rflags);
			if(send)
				g_gdbConnection->SendRawPacket({ packet, len });
			if (lastExceptionPacket)
				delete[] lastExceptionPacket;
			lastExceptionPacket = packet;
			lastExceptionPacketLen = len;
		}
		static void divisionError(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized)
			{
				notifyGdbOfException(frame);
				while (!HandlePacket(frame));
			}
			defaultExceptionHandler(frame);
		}
		static void debugException(interrupt_frame* frame)
		{
			// TODO: Implement this.
			defaultExceptionHandler(frame);
		}
		bool g_stubInitialized = false;
		static void breakpointInt(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23)
			{
				if (g_stubInitialized)
					notifyGdbOfException(frame);
				while(!HandlePacket(frame) || !g_stubInitialized);
			}
			defaultExceptionHandler(frame);
		}
		static void intOverflow(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized)
			{
				notifyGdbOfException(frame);
				while (!HandlePacket(frame));
			}
			defaultExceptionHandler(frame);
		}
		static void invalidOpcode(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized)
			{
				notifyGdbOfException(frame);
				while (!HandlePacket(frame));
			}
			defaultExceptionHandler(frame);
		}
		static void protectionFault(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized)
			{
				notifyGdbOfException(frame);
				while (!HandlePacket(frame));
			}
			defaultExceptionHandler(frame);
		}
		static void pageFault(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized)
			{
				if (frame->errorCode & 1 && ((uintptr_t)memory::getCurrentPageMap()->getL1PageMapEntryAt((uintptr_t)getCR2()) & ((uintptr_t)1 << 9)))
					goto done;
				notifyGdbOfException(frame);
				while (!HandlePacket(frame));
			}
			done:
			exception14(frame);
		}
		// Interrupts 16 and 20
		static void mathException(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized)
			{
				notifyGdbOfException(frame);
				while (!HandlePacket(frame));
			}
			defaultExceptionHandler(frame);
		}
		void InititalizeGDBStub(Connection* conn)
		{
			g_gdbConnection = conn;
			RegisterInterruptHandler(0, divisionError);
			RegisterInterruptHandler(1, debugException);
			// Skip exception 2 (nmi)
			RegisterInterruptHandler(3, breakpointInt);
			RegisterInterruptHandler(4, intOverflow);
			RegisterInterruptHandler(6, invalidOpcode);
			RegisterInterruptHandler(10, protectionFault);
			RegisterInterruptHandler(11, protectionFault);
			RegisterInterruptHandler(12, protectionFault);
			RegisterInterruptHandler(13, protectionFault);
			RegisterInterruptHandler(14, pageFault);
			RegisterInterruptHandler(16, mathException);
			RegisterInterruptHandler(21, mathException);
			while (!g_gdbConnection->CanReadByte());
			int3();
			g_stubInitialized = true;
		}
	}
}