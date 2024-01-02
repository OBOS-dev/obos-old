/*
	driverInterface/x86_64/scan.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <driverInterface/struct.h>
#include <driverInterface/load.h>

#include <driverInterface/x86_64/enumerate_pci.h>

#include <hashmap.h>
#include <vector.h>

#include <x86_64-utils/asm.h>

namespace obos
{
	namespace driverInterface
	{
		struct LoadableDriver
		{
			const char* fullPath = nullptr;
			driverHeader* header = nullptr;
			void operator=(const LoadableDriver& other)
			{
				fullPath = other.fullPath;
				header = other.header;
			}
		};
		bool operator==(const LoadableDriver& first, const LoadableDriver& second)
		{
			return utils::strcmp(first.fullPath, second.fullPath);
		}

		void ScanPCIBus(const utils::Vector<LoadableDriver>& driverList)
		{
			struct pciDevice
			{
				uint8_t classCode : 8;
				uint8_t subclass : 8;
				uint8_t progIF : 8;
			};
			utils::Hashmap<
				LoadableDriver, pciDevice> driversToLoad;
			uintptr_t udata[2] = { (uintptr_t)&driverList, (uintptr_t)&driversToLoad };
			for (uint16_t bus = 0; bus < 256; bus++)
				enumerateBus((uint8_t)bus, [](
					void* userData, uint8_t, uint8_t, uint8_t,
					uint8_t classCode,
					uint8_t subclass,
					uint8_t progIF)->bool
					{
						uintptr_t* udata = (uintptr_t*)userData;
						const utils::Vector<LoadableDriver>& driverList = *(utils::Vector<LoadableDriver>*)udata[0];
						utils::Hashmap<
							LoadableDriver, pciDevice>& driversToLoad = *((utils::Hashmap<
								LoadableDriver, pciDevice>*)udata[1]);
						pciDevice dev{
							classCode,subclass,progIF
						};
						for (auto& driver : driverList)
						{
							if (driversToLoad.contains(driver))
								continue;
							if (driver.header->howToIdentifyDevice & 1)
							{
								if (driver.header->pciInfo.classCode == dev.classCode)
								{
									bool found = true;
									for (uint8_t csubclass = bsf(driver.header->pciInfo.subclass); 
												 csubclass != subclass;
												 csubclass = bsf(driver.header->pciInfo.subclass))
									{
										if (!(driver.header->pciInfo.subclass & ((__uint128_t)1<<csubclass)))
										{
											found = false;
											break;
										}
									}
									if (!found)
										continue;
									for (uint8_t cprogIF = bsf(driver.header->pciInfo.progIf); 
												 cprogIF != progIF;
												 cprogIF = bsf(driver.header->pciInfo.progIf))
									{
										if (!(driver.header->pciInfo.progIf & ((__uint128_t)1<<cprogIF)))
										{
											found = false;
											break;
										}
									}
									driversToLoad.emplace_at(driver, dev);
									if (!found)
										continue;
								}
							}
						}
						return true;
					}, &udata);
			
		}
		void ScanAndLoadModules()
		{
			utils::Vector<LoadableDriver> drivers;
			ScanPCIBus(drivers);
		}
	}
}