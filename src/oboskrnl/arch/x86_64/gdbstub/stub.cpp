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
#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <arch/x86_64/interrupt.h>

#include <x86_64-utils/asm.h>

#include <vector.h>

#include <multitasking/cpu_local.h>
#include <multitasking/thread.h>
#include <multitasking/scheduler.h>

#include <multitasking/process/process.h>

#include <multitasking/threadAPI/thrHandle.h>

#define SET_REGISTER64(packetData, reg_) \
						if (!utils::memcmp(packetData, "xxxxxxxxxxxxxxxx", 16) && !utils::memcmp(currentPacket.data + 16, "XXXXXXXXXXXXXXXX", 16))\
							reg_ = bswap64(hex2bin(packetData, 16));\
						packetData += 16
#define SET_REGISTER32(packetData, reg_) \
						if (!utils::memcmp(packetData, "xxxxxxxxxxxxxxxx", 16) && !utils::memcmp(currentPacket.data + 16, "XXXXXXXXXXXXXXXX", 16))\
							reg_ = bswap32(hex2bin(packetData, 8));\
						packetData += 8

namespace obos
{
	extern uint8_t g_lapicIDs[256];
	extern uint8_t g_nCores;
	extern void exception14(interrupt_frame* frame);
	extern void defaultExceptionHandler(interrupt_frame* frame);
	void RegisterExceptionHandlers();
	extern void EarlyKPanic();
	namespace thread
	{
		void* lookForThreadInList(Thread::ThreadList& list, uint32_t tid);
	}
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
		enum dr7
		{
			DR6_B0_HIT=(1<<0),
			DR6_B1_HIT=(1<<1),
			DR6_B2_HIT=(1<<2),
			DR6_B3_HIT=(1<<3),
			DR6_SINGLE_STEP = (1<<14)
		};
		struct breakpoint
		{
			breakpoint *next, *prev;
			uintptr_t address;
			// If false, the structure refers a breakpoint, otherwise it refers a watchpoint.
			bool isWatchpoint : 1;
			// Is the watchpoint in DR0?
			bool isInDR0 : 1;
			// Is the watchpoint in DR1?
			bool isInDR1 : 1;
			// Is the watchpoint in DR2?
			bool isInDR2 : 1;
			// Is the watchpoint in DR3?
			bool isInDR3 : 1;
		};
		/*struct
		{
			breakpoint *head, *tail;
			size_t nBreakpoints;
		} g_breakpoints;*/
		static const char* setupRegisterResponse(interrupt_frame* frame)
		{
			constexpr const char* const format =
				"%e%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%x%e%x%x%x%x%x%x%x"
			    /*"%s%s%s%s%s%s%s%s" 
				"%s%s%s%s%s%s%s"*/;
			/*constexpr const char* const reg_empty = "xxxxxxxxxxxxxxxx";
			constexpr const char* const st_empty = "xxxxxxxxxxxxxxxxxxxx";
			constexpr const char* const fctrl_empty = "xxxx";
			constexpr const char* const seg_empty = "xxxx";
			constexpr const char* const opcode_empty = "xxxx";*/
			size_t len = logger::sprintf(nullptr, format, 16,
				bswap64(frame->rax), bswap64(frame->rbx), bswap64(frame->rcx),
				bswap64(frame->rdx), bswap64(frame->rsi), bswap64(frame->rdi),
				bswap64(frame->rbp), bswap64(frame->rsp), bswap64(frame->r8),
				bswap64(frame->r9), bswap64(frame->r10), bswap64(frame->r11),
				bswap64(frame->r12), bswap64(frame->r13), bswap64(frame->r14),
				bswap64(frame->r15), bswap64(frame->rip),
				8, // Padding to 8
				bswap32(frame->rflags.operator uintptr_t()),
				bswap32(frame->cs), bswap32(frame->ds), bswap32(frame->ds),
				bswap32(frame->ds), bswap32(frame->ds), bswap32(frame->ds)
				/*st_empty,st_empty,st_empty,st_empty,
				st_empty,st_empty,st_empty,st_empty,
				fctrl_empty,fctrl_empty,
				seg_empty, reg_empty,
				seg_empty, reg_empty,
				opcode_empty*/
				);
			char* response = new char[len + 1];
			logger::sprintf(response, format, 16,
				bswap64(frame->rax), bswap64(frame->rbx), bswap64(frame->rcx),
				bswap64(frame->rdx), bswap64(frame->rsi), bswap64(frame->rdi),
				bswap64(frame->rbp), bswap64(frame->rsp), bswap64(frame->r8),
				bswap64(frame->r9), bswap64(frame->r10), bswap64(frame->r11),
				bswap64(frame->r12), bswap64(frame->r13), bswap64(frame->r14),
				bswap64(frame->r15), bswap64(frame->rip),
				8, // Padding to 8
				bswap32(frame->rflags.operator uintptr_t()),
				bswap32(frame->cs), bswap32(frame->ds), bswap32(frame->ds),
				bswap32(frame->ds), bswap32(frame->ds), bswap32(frame->ds)
				/*st_empty,st_empty,st_empty,st_empty,
				st_empty,st_empty,st_empty,st_empty,
				fctrl_empty,fctrl_empty,
				seg_empty, reg_empty,
				seg_empty, reg_empty,
				opcode_empty*/
			);
			return response;
		}
		const char* lastExceptionPacket = nullptr;
		size_t lastExceptionPacketLen = 0;
		static uint64_t dec2bin(const char* str, size_t size)
		{
			uint32_t ret = 0;
			for (size_t i = 0; i < size; i++)
			{
				if ((str[i] - '0') < 0 || (str[i] - '0') > 9)
					continue;
				ret *= 10;
				ret += str[i] - '0';
			}
			return ret;
		}
		extern uintptr_t hex2bin(const char* str, unsigned size);
		enum class HandlePacketStatus
		{
			CONTINUE_HANDLING,
			CONTINUE_PROGRAM,
			ABORT,
		};
		static const thread::Thread* getCurrentThread()
		{
			if (thread::GetCurrentCpuLocalPtr())
				return (thread::Thread*)thread::GetCurrentCpuLocalPtr()->currentThread;
			return nullptr;
		}
		static uint32_t getTID()
		{
			const thread::Thread* thread = getCurrentThread();
			if (thread)
				return thread->tid;
			return 0;
		}
		static void step(thread::Thread* _thread, interrupt_frame* frame)
		{
			_thread->status |= thread::THREAD_STATUS_SINGLE_STEPPING;
			_thread->context.frame.rflags.setBit((uintptr_t)1 << 8);
			if (getCurrentThread() == _thread)
				frame->rflags.setBit((uintptr_t)1 << 8);
			else
				notifyGdbOfException(frame);
			// No need to send a new response if we're stepping the current thread, as the next debug exception will send a response.
		}
		static void _continue(thread::Thread* _thread, interrupt_frame* frame)
		{
			_thread->status &= ~(thread::THREAD_STATUS_PAUSED | thread::THREAD_STATUS_SINGLE_STEPPING);
			_thread->context.frame.rflags.setBit((uintptr_t)1 << 8);
			if (_thread == getCurrentThread())
				frame->rflags.setBit((uintptr_t)1 << 8);
			else
				notifyGdbOfException(frame);
			// No need to send a new response if we're continuing the current thread, as the next debug exception will send a response.
		}
		static void stopThr(thread::Thread* _thread, interrupt_frame* frame)
		{
			_thread->status |= thread::THREAD_STATUS_PAUSED;
			_thread->status &= thread::THREAD_STATUS_SINGLE_STEPPING;
			_thread->context.frame.rflags.setBit((uintptr_t)1 << 8);
			if (_thread == getCurrentThread())
				frame->rflags.setBit((uintptr_t)1 << 8);
			notifyGdbOfException(frame);
		}
		static HandlePacketStatus HandlePacket(interrupt_frame* frame)
		{
			Packet currentPacket{};
			HandlePacketStatus status = HandlePacketStatus::CONTINUE_HANDLING;
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
				case 'g':
				{
					const char* _response = nullptr;
					size_t _responseLen = 0;
					_response = setupRegisterResponse(frame);
					_responseLen = utils::strlen(_response);
					for (size_t i = 0; i < _responseLen; i++)
						response.push_back(_response[i]);
					if(_response)
						delete[] _response;
					break;
				}
				case 'H':
				{
					char operation = currentPacket.data[1];
					const char* _response = nullptr;
					size_t _responseLen = 0;
					bool shouldFree = false;
					uint32_t tid = -1;
					//bool chooseAllThreads = false;
					char* _tid = currentPacket.data + 2;
					size_t _sizeTid = currentPacket.len - 2;
					//chooseAllThreads = utils::memcmp(_tid, "-1", 2);
					if (utils::memcmp(_tid, "0", 1))
						tid = getTID(); // Choose the current thread.
					if (utils::strCountToChar(_tid, '.') < _sizeTid)
					{
						_sizeTid -= utils::strCountToChar(_tid, '.') + 1;
						_tid += utils::strCountToChar(_tid, '.') + 1;
					}
					if (*_tid != '0')
						tid = hex2bin(_tid, _sizeTid) - 1;
					else
						tid = getTID();
					thread::Thread* _thread = nullptr;
					if (tid != getTID())
					{
						_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[0], tid);
						if (!_thread)
							_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[1], tid);
						if (!_thread)
							_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[2], tid);
						if (!_thread)
							_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[3], tid);
						thread::cpu_local* cpuRunning = nullptr;
						for (size_t i = 0; i < thread::g_nCPUs && !cpuRunning; i++)
							if (thread::g_cpuInfo[i].currentThread == _thread)
								cpuRunning = thread::g_cpuInfo + i;
						_thread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_PAUSED;
						if (cpuRunning)
						{
							// Stop the thread from running.
							uint64_t localAPICId = cpuRunning->cpuId;
							g_localAPICAddr->interruptCommand0_31 = 0x30;
							g_localAPICAddr->interruptCommand32_63 = localAPICId << 56;
						}
					}
					else
						_thread = const_cast<thread::Thread*>(getCurrentThread());
					switch (operation)
					{
					case 'g':
					{
						// Setup the response.
						_response = setupRegisterResponse(getCurrentThread() == _thread ? frame : &_thread->context.frame);
						_responseLen = utils::strlen(_response);
						shouldFree = true;
						_thread->status &= ~thread::THREAD_STATUS_PAUSED;
						break;
					}
					case 'G':
					{
						char* data = ++currentPacket.data;
						if (_thread == getCurrentThread())
						{
							SET_REGISTER64(data, frame->rax);
							SET_REGISTER64(data, frame->rbx);
							SET_REGISTER64(data, frame->rcx);
							SET_REGISTER64(data, frame->rdx);
							SET_REGISTER64(data, frame->rsi);
							SET_REGISTER64(data, frame->rdi);
							SET_REGISTER64(data, frame->rbp);
							SET_REGISTER64(data, frame->rsp);
							SET_REGISTER64(data, frame->r8);
							SET_REGISTER64(data, frame->r9);
							SET_REGISTER64(data, frame->r10);
							SET_REGISTER64(data, frame->r11);
							SET_REGISTER64(data, frame->r12);
							SET_REGISTER64(data, frame->r13);
							SET_REGISTER64(data, frame->r14);
							SET_REGISTER64(data, frame->r15);
							SET_REGISTER64(data, frame->rip);
							SET_REGISTER32(data, frame->rflags);
							SET_REGISTER32(data, frame->cs);
							SET_REGISTER32(data, frame->ds);
						}
						data = currentPacket.data;
						SET_REGISTER64(data, _thread->context.frame.rax);
						SET_REGISTER64(data, _thread->context.frame.rbx);
						SET_REGISTER64(data, _thread->context.frame.rcx);
						SET_REGISTER64(data, _thread->context.frame.rdx);
						SET_REGISTER64(data, _thread->context.frame.rsi);
						SET_REGISTER64(data, _thread->context.frame.rdi);
						SET_REGISTER64(data, _thread->context.frame.rbp);
						SET_REGISTER64(data, _thread->context.frame.rsp);
						SET_REGISTER64(data, _thread->context.frame.r8);
						SET_REGISTER64(data, _thread->context.frame.r9);
						SET_REGISTER64(data, _thread->context.frame.r10);
						SET_REGISTER64(data, _thread->context.frame.r11);
						SET_REGISTER64(data, _thread->context.frame.r12);
						SET_REGISTER64(data, _thread->context.frame.r13);
						SET_REGISTER64(data, _thread->context.frame.r14);
						SET_REGISTER64(data, _thread->context.frame.r15);
						SET_REGISTER64(data, _thread->context.frame.rip);
						SET_REGISTER32(data, _thread->context.frame.rflags);
						SET_REGISTER32(data, _thread->context.frame.cs);
						SET_REGISTER32(data, _thread->context.frame.ds);
						_thread->status &= ~thread::THREAD_STATUS_PAUSED;
						break;
					}
					case 'c':
						_thread->status &= ~thread::THREAD_STATUS_PAUSED;
						if (_thread == getCurrentThread())
							status = HandlePacketStatus::CONTINUE_PROGRAM;
						_continue(_thread, frame);
						break;
					case 's':
						_thread->status &= ~thread::THREAD_STATUS_PAUSED;
						step(_thread, frame);
						if (_thread == getCurrentThread())
							status = HandlePacketStatus::CONTINUE_PROGRAM;
						break;
					default:
						break;
					}
					for (size_t i = 0; i < _responseLen; i++)
							response.push_back(_response[i]);
					if (shouldFree)
						delete[] _response;
					break;
				}
				case 'G':
				{
					char* data = ++currentPacket.data;
					SET_REGISTER64(data, frame->rax);
					SET_REGISTER64(data, frame->rbx);
					SET_REGISTER64(data, frame->rcx);
					SET_REGISTER64(data, frame->rdx);
					SET_REGISTER64(data, frame->rsi);
					SET_REGISTER64(data, frame->rdi);
					SET_REGISTER64(data, frame->rbp);
					SET_REGISTER64(data, frame->rsp);
					SET_REGISTER64(data, frame->r8);
					SET_REGISTER64(data, frame->r9);
					SET_REGISTER64(data, frame->r10);
					SET_REGISTER64(data, frame->r11);
					SET_REGISTER64(data, frame->r12);
					SET_REGISTER64(data, frame->r13);
					SET_REGISTER64(data, frame->r14);
					SET_REGISTER64(data, frame->r15);
					SET_REGISTER64(data, frame->rip);
					SET_REGISTER32(data, frame->rflags);
					SET_REGISTER32(data, frame->cs);
					SET_REGISTER32(data, frame->ds);
					break;
				}
				case 'm':
				{
					// Get the address and the amount to read.
					uint64_t rAddr = 0;
					size_t nToRead = 0;
					bool gran32 = false;
					rAddr = hex2bin(currentPacket.data + 1, utils::strCountToChar(currentPacket.data + 1, ','));
					nToRead = dec2bin(currentPacket.data + utils::strCountToChar(currentPacket.data + 1, ',') + 2, currentPacket.len - utils::strCountToChar(currentPacket.data + 1, ',') - 2);
					// The LAPIC and HPET require 32-bit granularity reads.
					gran32 = rAddr >= 0xffffffffffffd000 && rAddr < 0xfffffffffffff000;
					if (gran32)
					{
						nToRead = (nToRead / 4 + 1) * 4;
						rAddr &= ~4;
					}
					size_t nPagesToRead = ((nToRead + 0xfff) & ~0xfff) / 4096;
					uintptr_t* protFlags = new uintptr_t[nPagesToRead];
					memory::VirtualGetProtection((void*)(rAddr & ~0xfff), nPagesToRead, protFlags);
					bool canRead = true;
					for (size_t i = 0; i < nPagesToRead && canRead; canRead = canRead && protFlags[i++] & memory::PROT_IS_PRESENT);
					if(!canRead)
						break;
					char* _response = new char[nToRead];
					if (gran32)
						utils::dwMemcpy((uint32_t*)_response, (uint32_t*)rAddr, nToRead / 4 + 1);
					else
						utils::memcpy(_response, (void*)rAddr, nToRead);
					for (size_t i = 0; i < nToRead; i++)
					{
						constexpr const char* const hextable = "0123456789abcdef";
						char hex[2] = { hextable[(_response[i] >> 4 & 0xf)], hextable[_response[i] & 0xf] };
						response.push_back(hex[0] ? hex[0] : '0');
						response.push_back(hex[1] ? hex[1] : '0');
					}
					delete[] _response;
					break;
				}
				case 'k':
				{
					// Shutdown the system if in an emulator.
					if (inb(0xE9) == 0xE9)
						outw(0xB004, 0x2000); // Bochs
					char cpuManufacturer[13] = {};
					uint32_t unused = 0;
					__cpuid__(0, 0, &unused, ((uint32_t*)cpuManufacturer), ((uint32_t*)cpuManufacturer) + 2, ((uint32_t*)cpuManufacturer) + 1);
					if (utils::memcmp(cpuManufacturer, "TCGTCGTCGTCG", 12))
						outw(0x604, 0x2000); // Qemu
					if (utils::memcmp(cpuManufacturer, "VBoxVBoxVBox", 12))
						outw(0x4004, 0x3400); // VirtualBox
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
				case 'v':
				{
					char* packetData = currentPacket.data + 1;
					char* command = new char[utils::strCountToChar(packetData, ';') + 1];
					utils::memcpy(command, packetData, utils::strCountToChar(packetData, ';'));
					char* _response = nullptr;
					size_t _responseLen = 0;
					bool shouldFree = false;
					if (utils::strcmp(command, "Cont?"))
					{
						_response = (char*)"vCont;c;s;t";
						_responseLen = 9;
					}
					else if (utils::strcmp(command, "Cont"))
					{
						_response = (char*)"";
						_responseLen = 1;
						while (packetData < (currentPacket.data + currentPacket.len))
						{
							uint32_t tid = -1;
							bool chooseAllThreads = false;
							char action = packetData[utils::strCountToChar(packetData, ';') + 1];
							packetData++;
							if (packetData[utils::strCountToChar(packetData, ';')] != ';')
							{
								char* _tid = packetData + utils::strCountToChar(packetData, ':') + 1;
								size_t _sizeTid = utils::strCountToChar(_tid, ';');
								chooseAllThreads = utils::memcmp(_tid, "-1", 2);
								if (utils::strCountToChar(_tid, '.') < _sizeTid)
								{
									_sizeTid -= utils::strCountToChar(_tid, '.') + 1;
									_tid += utils::strCountToChar(_tid, '.') + 1;
								}
								if (*_tid != '0')
									tid = hex2bin(_tid, _sizeTid);
								else
									tid = getTID();
								packetData = _tid + _sizeTid + 1;
							}
							if (!chooseAllThreads && tid == (uint32_t)-1)
								tid = getTID(); // Choose the current thread.
							else
								tid--;
							thread::Thread* _thread = nullptr;
							if (tid != getTID())
							{
								_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[0], tid);
								if (!_thread)
									_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[1], tid);
								if (!_thread)
									_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[2], tid);
								if (!_thread)
									_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[3], tid);
								thread::cpu_local* cpuRunning = nullptr;
								for (size_t i = 0; i < thread::g_nCPUs && !cpuRunning; i++)
									if (thread::g_cpuInfo[i].currentThread == _thread)
										cpuRunning = thread::g_cpuInfo + i;
								_thread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_PAUSED;
								if (cpuRunning)
								{
									// Stop the thread from running.
									uint64_t localAPICId = cpuRunning->cpuId;
									g_localAPICAddr->interruptCommand0_31 = 0x30;
									g_localAPICAddr->interruptCommand32_63 = localAPICId << 56;
								}
							}
							else
								_thread = const_cast<thread::Thread*>(getCurrentThread());
							switch (action)
							{
							case 'c':
								_thread->status &= ~thread::THREAD_STATUS_PAUSED;
								if (_thread == getCurrentThread())
									status = HandlePacketStatus::CONTINUE_PROGRAM;
								_continue(_thread, frame);
								break;
							case 's':
								_thread->status &= ~thread::THREAD_STATUS_PAUSED;
								step(_thread, frame);
								if (_thread == getCurrentThread())
									status = HandlePacketStatus::CONTINUE_PROGRAM;
								break;
							case 't':
								stopThr(_thread, frame);
								break;
							default:
								break;
							}
						}
					}
					for (size_t i = 0; i < _responseLen; i++)
						response.push_back(_response[i]);
					if (shouldFree)
						delete[] _response;
					delete[] command;
					break;
				}
				case 's':
				{
					thread::Thread* _thread = const_cast<thread::Thread*>(getCurrentThread());
					_thread->status &= ~thread::THREAD_STATUS_PAUSED;
					status = HandlePacketStatus::CONTINUE_PROGRAM;
					step(_thread, frame);
					break;
				}
				case 'c':
				{
					thread::Thread* _thread = const_cast<thread::Thread*>(getCurrentThread());
					_thread->status &= ~thread::THREAD_STATUS_PAUSED;
					status = HandlePacketStatus::CONTINUE_PROGRAM;
					_continue(_thread, frame);
					break;
				}
				case 'q':
				{
					char* packetData = currentPacket.data + 1;
					char* command = nullptr;
					size_t commandLen = utils::strCountToChar(packetData, ':');
					if (utils::strCountToChar(packetData, ':') != currentPacket.len - 1)
						command = new char[commandLen = (utils::strCountToChar(packetData, ':') + 1)];
					else if (utils::strCountToChar(packetData, ',') != currentPacket.len - 1)
						command = new char[commandLen = (utils::strCountToChar(packetData, ',') + 1)];
					else
						command = new char[commandLen = (utils::strlen(packetData) + 1)];
					utils::memcpy(command, packetData, commandLen);
					char* _response = nullptr;
					size_t _responseLen = 0;
					bool shouldFree = false;
					if (utils::strcmp(command, "Supported"))
					{
						shouldFree = true;
						packetData += utils::strCountToChar(packetData, ':') + 1;
						while (packetData < (currentPacket.data + currentPacket.len))
						{
							size_t featureLen = utils::strCountToChar(packetData, ';');
							const char* feature = (char*)utils::memcpy(new char[featureLen + 1], packetData, featureLen);
							const char* currentResponse = "";
							size_t newLen = _responseLen;
							if (utils::strcmp(feature, "hwbreak+"))
							{
								currentResponse = "hwbreak+";
								newLen += 8;
							}
							else if (utils::strcmp(feature, "swbreak+"))
							{
								currentResponse = "swbreak+";
								newLen += 8;
							}
							else if (utils::strcmp(feature, "vContSupported+"))
							{
								currentResponse = "vContSupported+";
								newLen += 15;
							}
							else if (utils::strcmp(feature, "multiprocess+"))
							{
								currentResponse = "multiprocess+";
								newLen += 13;
							}
							if (newLen)
							{
								_response = (char*)krealloc(_response, newLen);
								utils::memcpy(_response + _responseLen, currentResponse, utils::strlen(currentResponse));
							}
							_responseLen = newLen;
							delete[] feature;
							packetData += utils::strCountToChar(packetData, ';') + 1;
						}
					}
					else if (utils::strcmp(command, "fThreadInfo"))
					{
						// We debug all threads cs == 0x08 and ds == 0x10
						_response = (char*)utils::memcpy(new char[2], "m", 1);
						_responseLen = 1;
						shouldFree = true;
						auto scanThreadList =
							[&](thread::Thread::ThreadList& list) {
							thread::Thread* currentThread = list.head;
							while (currentThread)
							{
								if (currentThread->context.frame.cs == 0x08 && currentThread->context.frame.ds == 0x10)
								{
									constexpr const char* const format = "%ep%x.%x,";
									size_t newLen = _responseLen + logger::sprintf(nullptr, format, 8, 0,0);
									_response = (char*)krealloc(_response, newLen);
									logger::sprintf(_response + _responseLen, format, 8, ((process::Process*)currentThread->owner)->pid + 1, currentThread->tid + 1);
									_responseLen = newLen;
								}
								currentThread = currentThread->next_run;
							}
							};
						scanThreadList(thread::g_priorityLists[0]);
						scanThreadList(thread::g_priorityLists[1]);
						scanThreadList(thread::g_priorityLists[2]);
						scanThreadList(thread::g_priorityLists[3]);
						_response[_responseLen - 1] = 'T'; // Replace the trailing comma with a 'T'.
					}
					else if (utils::strcmp(command, "ThreadExtraInfo"))
					{
						// Get the thread id.
						shouldFree = true;
						uint32_t tid = -1;
						//bool chooseAllThreads = false;
						char* _tid = currentPacket.data + commandLen + 1;
						size_t _sizeTid = currentPacket.len - commandLen - 1;
						//chooseAllThreads = utils::memcmp(_tid, "-1", 2);
						if (utils::memcmp(_tid, "0", 1))
							tid = getTID(); // Choose the current thread.
						if (utils::strCountToChar(_tid, '.') < _sizeTid)
						{
							_sizeTid -= utils::strCountToChar(_tid, '.') + 1;
							_tid += utils::strCountToChar(_tid, '.') + 1;
						}
						if (*_tid != '0')
							tid = hex2bin(_tid, _sizeTid) - 1;
						else
							tid = getTID();
						thread::Thread* _thread = nullptr;
						if (tid != getTID())
						{
							_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[0], tid);
							if (!_thread)
								_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[1], tid);
							if (!_thread)
								_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[2], tid);
							if (!_thread)
								_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[3], tid);
							thread::cpu_local* cpuRunning = nullptr;
							for (size_t i = 0; i < thread::g_nCPUs && !cpuRunning; i++)
								if (thread::g_cpuInfo[i].currentThread == _thread)
									cpuRunning = thread::g_cpuInfo + i;
							_thread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_PAUSED;
							if (cpuRunning)
							{
								// Stop the thread from running.
								uint64_t localAPICId = cpuRunning->cpuId;
								g_localAPICAddr->interruptCommand0_31 = 0x30;
								g_localAPICAddr->interruptCommand32_63 = localAPICId << 56;
							}
						}
						else
							_thread = const_cast<thread::Thread*>(getCurrentThread());
						constexpr const char* const statusStrTable[] = {
							"THREAD_STATUS_DEAD (0b1) | ",
							"THREAD_STATUS_CAN_RUN (0b10) | ",
							"THREAD_STATUS_BLOCKED (0b100) | ",
							"THREAD_STATUS_PAUSED (0b1000) | ",
							"THREAD_STATUS_CLEAR_TIME_SLICE_INDEX (0b10000) | ",
							"THREAD_STATUS_RUNNING (0b100000) | ",
							"THREAD_STATUS_CALLING_BLOCK_CALLBACK (0b1000000) | ",
							"THREAD_STATUS_SINGLE_STEPPING (0b10000000) | ",
						};
						if (!_thread)
						{
							_response = (char*)"No such thread.";
							_responseLen = 15;
							goto end;
						}
						if (_thread->status & thread::THREAD_STATUS_DEAD)
						{
							size_t newSize = _responseLen + utils::strlen(statusStrTable[0]);
							_response = (char*)utils::memcpy(krealloc(_response, newSize + 1), statusStrTable[0], newSize - _responseLen);
						}
						if (_thread->status & thread::THREAD_STATUS_CAN_RUN)
						{
							size_t newSize = _responseLen + utils::strlen(statusStrTable[1]);
							_response = (char*)utils::memcpy(krealloc(_response, newSize + 1), statusStrTable[1], newSize - _responseLen);
						}
						if (_thread->status & thread::THREAD_STATUS_BLOCKED)
						{
							size_t newSize = _responseLen + utils::strlen(statusStrTable[2]);
							_response = (char*)utils::memcpy(krealloc(_response, newSize + 1), statusStrTable[2], newSize - _responseLen);
						}
						if (_thread->status & thread::THREAD_STATUS_PAUSED)
						{
							size_t newSize = _responseLen + utils::strlen(statusStrTable[3]);
							_response = (char*)utils::memcpy(krealloc(_response, newSize + 1), statusStrTable[3], newSize - _responseLen);
						}
						if (_thread->status & thread::THREAD_STATUS_CLEAR_TIME_SLICE_INDEX)
						{
							size_t newSize = _responseLen + utils::strlen(statusStrTable[4]);
							_response = (char*)utils::memcpy(krealloc(_response, newSize + 1), statusStrTable[4], newSize - _responseLen);
						}
						if (_thread->status & thread::THREAD_STATUS_RUNNING)
						{
							size_t newSize = _responseLen + utils::strlen(statusStrTable[5]);
							_response = (char*)utils::memcpy(krealloc(_response, newSize + 1), statusStrTable[5], newSize - _responseLen);
						}
						if (_thread->status & thread::THREAD_STATUS_CALLING_BLOCK_CALLBACK)
						{
							size_t newSize = _responseLen + utils::strlen(statusStrTable[6]);
							_response = (char*)utils::memcpy(krealloc(_response, newSize + 1), statusStrTable[6], newSize - _responseLen);
						}
						if (_thread->status & thread::THREAD_STATUS_SINGLE_STEPPING)
						{
							size_t newSize = _responseLen + utils::strlen(statusStrTable[7]);
							_response = (char*)utils::memcpy(krealloc(_response, newSize + 1), statusStrTable[7], newSize - _responseLen);
						}
						if (_responseLen)
							_responseLen -= 3; // Remove the trailing " | " from the string.
					end:
						(void)0;
					}
					for (size_t i = 0; i < _responseLen; i++)
						response.push_back(_response[i]);
					if(shouldFree && _response != nullptr)
						delete[] _response;
					delete[] command;
					break;
				}
				case 'T':
				{
					uint32_t tid = -1;
					//bool chooseAllThreads = false;
					char* _tid = currentPacket.data + 1;
					size_t _sizeTid = currentPacket.len - 1;
					//chooseAllThreads = utils::memcmp(_tid, "-1", 2);
					if (utils::memcmp(_tid, "0", 1))
						tid = getTID(); // Choose the current thread.
					if (utils::strCountToChar(_tid, '.') < _sizeTid)
					{
						_sizeTid -= utils::strCountToChar(_tid, '.') + 1;
						_tid += utils::strCountToChar(_tid, '.') + 1;
					}
					if (*_tid != '0')
						tid = hex2bin(_tid, _sizeTid) - 1;
					else
						tid = getTID();
					thread::Thread* _thread = nullptr;
					if (tid != getTID())
					{
						_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[0], tid);
						if (!_thread)
							_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[1], tid);
						if (!_thread)
							_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[2], tid);
						if (!_thread)
							_thread = (thread::Thread*)thread::lookForThreadInList(thread::g_priorityLists[3], tid);
						thread::cpu_local* cpuRunning = nullptr;
						for (size_t i = 0; i < thread::g_nCPUs && !cpuRunning; i++)
							if (thread::g_cpuInfo[i].currentThread == _thread)
								cpuRunning = thread::g_cpuInfo + i;
						_thread->status = thread::THREAD_STATUS_CAN_RUN | thread::THREAD_STATUS_PAUSED;
						if (cpuRunning)
						{
							// Stop the thread from running.
							uint64_t localAPICId = cpuRunning->cpuId;
							g_localAPICAddr->interruptCommand0_31 = 0x30;
							g_localAPICAddr->interruptCommand32_63 = localAPICId << 56;
						}
					}
					else
						_thread = const_cast<thread::Thread*>(getCurrentThread());
					if (_thread && _thread->status != thread::THREAD_STATUS_DEAD)
					{
						response.push_back('O');
						response.push_back('K');
					}
					else
					{
						// The thread doesn't exist, or is dead.
						response.push_back('E');
						response.push_back('0');
						response.push_back('0');
					}
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
			return status;
		}
		static void notifyGdbOfException(interrupt_frame* frame, bool send)
		{
			// 4 - SIGILL
			// 5 - SIGTRAP
			// 8 - SIGFPE
			// 11 - SIGSEGV
			constexpr uint8_t exceptionToSignal[] = {
				8, 5, 0, 5, 8, 0, 4, 0, // Exceptions 0-7
				0, 0,11,11,11,11,11, 0, // Exceptions 8-15
				8, 0, 0, 0, 0, 8, 0, 0, // Exceptions 16-23
			};
			uint8_t signalN = exceptionToSignal[frame->intNumber];
			constexpr const char* const format = "%eT%x%ethread:%x;";
			uint32_t tid = getTID() + 1;
			/*constexpr uint8_t padding = 8;*/
			size_t len = 
			logger::sprintf(nullptr, format, 2, signalN, 8, tid);
			char* packet = new char[len + 1];
			logger::sprintf(packet , format, 2, signalN, 8, tid);
			/*logger::sprintf(nullptr, format, 2, signalN, 8, tid, padding, RIP, 16, frame->rip, padding, RSP, 16, frame->rsp, 8, RFL, frame->rflags);
			char* packet = new char[len + 1];
			logger::sprintf(packet , format, 2, signalN, 8, tid, padding, RIP, 16, frame->rip, padding, RSP, 16, frame->rsp, 8, RFL, frame->rflags);*/
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
				while (HandlePacket(frame) == HandlePacketStatus::CONTINUE_HANDLING);
			}
			defaultExceptionHandler(frame);
		}
		static void debugException(interrupt_frame* frame)
		{
			// TODO: Implement this.
			if (frame->cs != 0x1f && frame->ds != 0x23)
			{
				uint64_t dr6 = getDR6();
				if (dr6 & DR6_SINGLE_STEP && getCurrentThread()->status & thread::THREAD_STATUS_SINGLE_STEPPING)
					notifyGdbOfException(frame);
				HandlePacketStatus status = HandlePacketStatus::CONTINUE_HANDLING;
				for (status = HandlePacket(frame); status == HandlePacketStatus::CONTINUE_HANDLING; status = HandlePacket(frame));
				switch (status)
				{
				case obos::gdbstub::HandlePacketStatus::CONTINUE_PROGRAM:
					return;
				case obos::gdbstub::HandlePacketStatus::ABORT:
					break;
				default:
					break;
				}
			}
			defaultExceptionHandler(frame);
		}
		bool g_stubInitialized = false;
		static void breakpointInt(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23)
			{
				if (g_stubInitialized)
					notifyGdbOfException(frame);
				HandlePacketStatus status = HandlePacketStatus::CONTINUE_HANDLING;
				for (status = HandlePacket(frame); status == HandlePacketStatus::CONTINUE_HANDLING; status = HandlePacket(frame));
				if (!g_stubInitialized)
					g_stubInitialized = true;
				switch (status)
				{
				case obos::gdbstub::HandlePacketStatus::CONTINUE_PROGRAM:
					return;
				case obos::gdbstub::HandlePacketStatus::ABORT:
					break;
				default:
					break;
				}
			}
			defaultExceptionHandler(frame);
		}
		static void intOverflow(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized)
			{
				notifyGdbOfException(frame);
				HandlePacketStatus status = HandlePacketStatus::CONTINUE_HANDLING;
				for (status = HandlePacket(frame); status == HandlePacketStatus::CONTINUE_HANDLING; status = HandlePacket(frame));
				switch (status)
				{
				case obos::gdbstub::HandlePacketStatus::CONTINUE_PROGRAM:
					return;
				case obos::gdbstub::HandlePacketStatus::ABORT:
					break;
				default:
					break;
				}
			}
			defaultExceptionHandler(frame);
		}
		static void invalidOpcode(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized)
			{
				notifyGdbOfException(frame);
				HandlePacketStatus status = HandlePacketStatus::CONTINUE_HANDLING;
				for (status = HandlePacket(frame); status == HandlePacketStatus::CONTINUE_HANDLING; status = HandlePacket(frame));
				switch (status)
				{
				case obos::gdbstub::HandlePacketStatus::CONTINUE_PROGRAM:
					return;
				case obos::gdbstub::HandlePacketStatus::ABORT:
					break;
				default:
					break;
				}
			}
			defaultExceptionHandler(frame);
		}
		static void protectionFault(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized)
			{
				notifyGdbOfException(frame);
				HandlePacketStatus status = HandlePacketStatus::CONTINUE_HANDLING;
				for (status = HandlePacket(frame); status == HandlePacketStatus::CONTINUE_HANDLING; status = HandlePacket(frame));
				switch (status)
				{
				case obos::gdbstub::HandlePacketStatus::CONTINUE_PROGRAM:
					return;
				case obos::gdbstub::HandlePacketStatus::ABORT:
					break;
				default:
					break;
				}
			}
			defaultExceptionHandler(frame);
		}
		static void pageFault(interrupt_frame* frame)
		{
			if (frame->cs != 0x1f && frame->ds != 0x23 && g_stubInitialized && false)
			{
				if (frame->errorCode & 1 && ((uintptr_t)memory::getCurrentPageMap()->getL1PageMapEntryAt((uintptr_t)getCR2()) & ((uintptr_t)1 << 9)))
					goto done;
				notifyGdbOfException(frame);
				HandlePacketStatus status = HandlePacketStatus::CONTINUE_HANDLING;
				for (status = HandlePacket(frame); status == HandlePacketStatus::CONTINUE_HANDLING; status = HandlePacket(frame));
				switch (status)
				{
				case obos::gdbstub::HandlePacketStatus::CONTINUE_PROGRAM:
					return;
				case obos::gdbstub::HandlePacketStatus::ABORT:
					break;
				default:
					break;
				}
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
				HandlePacketStatus status = HandlePacketStatus::CONTINUE_HANDLING;
				for (status = HandlePacket(frame); status == HandlePacketStatus::CONTINUE_HANDLING; status = HandlePacket(frame));
				switch (status)
				{
				case obos::gdbstub::HandlePacketStatus::CONTINUE_PROGRAM:
					return;
				case obos::gdbstub::HandlePacketStatus::ABORT:
					break;
				default:
					break;
				}
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
			logger::info("Connecting to gdb...");
			while (!g_gdbConnection->CanReadByte());
			int3();
			logger::info("Connected to gdb!");
			while (1);
		}
	}
}