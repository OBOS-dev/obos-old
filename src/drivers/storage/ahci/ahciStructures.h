/*
	drivers/storage/ahci/ahciStrucutures.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#define	SATA_SIG_ATA	0x00000101	// SATA drive
#define	SATA_SIG_ATAPI	0xEB140101	// SATAPI drive
#define	SATA_SIG_SEMB	0xC33C0101	// Enclosure management bridge
#define	SATA_SIG_PM	0x96690101	// Port multiplier

#define HBA_PORT_IPM_ACTIVE 1
#define HBA_PORT_DET_PRESENT 3

typedef enum
{
	FIS_TYPE_REG_H2D = 0x27,	// Register FIS - host to device
	FIS_TYPE_REG_D2H = 0x34,	// Register FIS - device to host
	FIS_TYPE_DMA_ACT = 0x39,	// DMA activate FIS - device to host
	FIS_TYPE_DMA_SETUP = 0x41,	// DMA setup FIS - bidirectional
	FIS_TYPE_DATA = 0x46,	// Data FIS - bidirectional
	FIS_TYPE_BIST = 0x58,	// BIST activate FIS - bidirectional
	FIS_TYPE_PIO_SETUP = 0x5F,	// PIO setup FIS - device to host
	FIS_TYPE_DEV_BITS = 0xA1,	// Set device bits FIS - device to host
} FIS_TYPE;

typedef struct tagFIS_REG_H2D
{
	// DWORD 0
	UINT8_T  fis_type;	// FIS_TYPE_REG_H2D

	UINT8_T  pmport : 4;	// Port multiplier
	UINT8_T  rsv0 : 3;		// Reserved
	UINT8_T  c : 1;		// 1: Command, 0: Control

	UINT8_T  command;	// Command register
	UINT8_T  featurel;	// Feature register, 7:0

	// DWORD 1
	UINT8_T  lba0;		// LBA low register, 7:0
	UINT8_T  lba1;		// LBA mid register, 15:8
	UINT8_T  lba2;		// LBA high register, 23:16
	UINT8_T  device;		// Device register

	// DWORD 2
	UINT8_T  lba3;		// LBA register, 31:24
	UINT8_T  lba4;		// LBA register, 39:32
	UINT8_T  lba5;		// LBA register, 47:40
	UINT8_T  featureh;	// Feature register, 15:8

	// DWORD 3
	UINT8_T  countl;		// Count register, 7:0
	UINT8_T  counth;		// Count register, 15:8
	UINT8_T  icc;		// Isochronous command completion
	UINT8_T  control;	// Control register

	// DWORD 4
	UINT8_T  rsv1[4];	// Reserved
} FIS_REG_H2D;

typedef struct tagFIS_REG_D2H
{
	// DWORD 0
	UINT8_T  fis_type;    // FIS_TYPE_REG_D2H

	UINT8_T  pmport : 4;    // Port multiplier
	UINT8_T  rsv0 : 2;      // Reserved
	UINT8_T  i : 1;         // Interrupt bit
	UINT8_T  rsv1 : 1;      // Reserved

	UINT8_T  status;      // Status register
	UINT8_T  error;       // Error register

	// DWORD 1
	UINT8_T  lba0;        // LBA low register, 7:0
	UINT8_T  lba1;        // LBA mid register, 15:8
	UINT8_T  lba2;        // LBA high register, 23:16
	UINT8_T  device;      // Device register

	// DWORD 2
	UINT8_T  lba3;        // LBA register, 31:24
	UINT8_T  lba4;        // LBA register, 39:32
	UINT8_T  lba5;        // LBA register, 47:40
	UINT8_T  rsv2;        // Reserved

	// DWORD 3
	UINT8_T  countl;      // Count register, 7:0
	UINT8_T  counth;      // Count register, 15:8
	UINT8_T  rsv3[2];     // Reserved

	// DWORD 4
	UINT8_T  rsv4[4];     // Reserved
} FIS_REG_D2H;

typedef struct tagFIS_DATA
{
	// DWORD 0
	UINT8_T  fis_type;	// FIS_TYPE_DATA

	UINT8_T  pmport : 4;	// Port multiplier
	UINT8_T  rsv0 : 4;		// Reserved

	UINT8_T  rsv1[2];	// Reserved

	// DWORD 1 ~ N
	UINT32_T data[1];	// Payload
} FIS_DATA;

typedef struct tagFIS_PIO_SETUP
{
	// DWORD 0
	UINT8_T  fis_type;	// FIS_TYPE_PIO_SETUP

	UINT8_T  pmport : 4;	// Port multiplier
	UINT8_T  rsv0 : 1;		// Reserved
	UINT8_T  d : 1;		// Data transfer direction, 1 - device to host
	UINT8_T  i : 1;		// Interrupt bit
	UINT8_T  rsv1 : 1;

	UINT8_T  status;		// Status register
	UINT8_T  error;		// Error register

	// DWORD 1
	UINT8_T  lba0;		// LBA low register, 7:0
	UINT8_T  lba1;		// LBA mid register, 15:8
	UINT8_T  lba2;		// LBA high register, 23:16
	UINT8_T  device;		// Device register

	// DWORD 2
	UINT8_T  lba3;		// LBA register, 31:24
	UINT8_T  lba4;		// LBA register, 39:32
	UINT8_T  lba5;		// LBA register, 47:40
	UINT8_T  rsv2;		// Reserved

	// DWORD 3
	UINT8_T  countl;		// Count register, 7:0
	UINT8_T  counth;		// Count register, 15:8
	UINT8_T  rsv3;		// Reserved
	UINT8_T  e_status;	// New value of status register

	// DWORD 4
	UINT16_T tc;		// Transfer count
	UINT8_T  rsv4[2];	// Reserved
} FIS_PIO_SETUP;

typedef struct tagFIS_DMA_SETUP
{
	// DWORD 0
	UINT8_T  fis_type;	// FIS_TYPE_DMA_SETUP

	UINT8_T  pmport : 4;	// Port multiplier
	UINT8_T  rsv0 : 1;		// Reserved
	UINT8_T  d : 1;		// Data transfer direction, 1 - device to host
	UINT8_T  i : 1;		// Interrupt bit
	UINT8_T  a : 1;            // Auto-activate. Specifies if DMA Activate FIS is needed

	UINT8_T  rsved[2];       // Reserved

	//DWORD 1&2

	UINT64_T DMAbufferID;    // DMA Buffer Identifier. Used to Identify DMA buffer in host memory.
	// SATA Spec says host specific and not in Spec. Trying AHCI spec might work.

//DWORD 3
	UINT32_T rsvd;           //More reserved

	//DWORD 4
	UINT32_T DMAbufOffset;   //Byte offset into buffer. First 2 bits must be 0

	//DWORD 5
	UINT32_T TransferCount;  //Number of bytes to transfer. Bit 0 must be 0

	//DWORD 6
	UINT32_T resvd;          //Reserved

} FIS_DMA_SETUP;

typedef volatile struct tagHBA_PORT
{
	UINT32_T clb;		// 0x00, command list base address, 1K-byte aligned
	UINT32_T clbu;		// 0x04, command list base address upper 32 bits
	UINT32_T fb;		// 0x08, FIS base address, 256-byte aligned
	UINT32_T fbu;		// 0x0C, FIS base address upper 32 bits
	UINT32_T is;		// 0x10, interrupt status
	UINT32_T ie;		// 0x14, interrupt enable
	UINT32_T cmd;		// 0x18, command and status
	UINT32_T rsv0;		// 0x1C, Reserved
	UINT32_T tfd;		// 0x20, task file data
	UINT32_T sig;		// 0x24, signature
	UINT32_T ssts;		// 0x28, SATA status (SCR0:SStatus)
	UINT32_T sctl;		// 0x2C, SATA control (SCR2:SControl)
	UINT32_T serr;		// 0x30, SATA error (SCR1:SError)
	UINT32_T sact;		// 0x34, SATA active (SCR3:SActive)
	UINT32_T ci;		// 0x38, command issue
	UINT32_T sntf;		// 0x3C, SATA notification (SCR4:SNotification)
	UINT32_T fbs;		// 0x40, FIS-based switch control
	UINT32_T rsv1[11];	// 0x44 ~ 0x6F, Reserved
	UINT32_T vendor[4];	// 0x70 ~ 0x7F, vendor specific
} HBA_PORT;

typedef volatile struct tagHBA_MEM
{
	// 0x00 - 0x2B, Generic Host Control
	UINT32_T cap;		// 0x00, Host capability
	UINT32_T ghc;		// 0x04, Global host control
	UINT32_T is;		// 0x08, Interrupt status
	UINT32_T pi;		// 0x0C, Port implemented
	UINT32_T vs;		// 0x10, Version
	UINT32_T ccc_ctl;	// 0x14, Command completion coalescing control
	UINT32_T ccc_pts;	// 0x18, Command completion coalescing ports
	UINT32_T em_loc;		// 0x1C, Enclosure management location
	UINT32_T em_ctl;		// 0x20, Enclosure management control
	UINT32_T cap2;		// 0x24, Host capabilities extended
	UINT32_T bohc;		// 0x28, BIOS/OS handoff control and status

	// 0x2C - 0x9F, Reserved
	UINT8_T  rsv[0xA0 - 0x2C];

	// 0xA0 - 0xFF, Vendor specific registers
	UINT8_T  vendor[0x100 - 0xA0];

	// 0x100 - 0x10FF, Port control registers
	HBA_PORT	ports[1];	// 1 ~ 32
} HBA_MEM;

typedef volatile struct tagFIS_DEV_BITS
{
	UINT8_T fisType;
	UINT8_T flags;
	UINT8_T status;
	UINT8_T error;
	UINT32_T protocol;
} FIS_DEV_BITS;

typedef volatile struct tagHBA_FIS
{
	// 0x00
	FIS_DMA_SETUP	dsfis;		// DMA Setup FIS
	UINT8_T         pad0[4];

	// 0x20
	FIS_PIO_SETUP	psfis;		// PIO Setup FIS
	UINT8_T         pad1[12];

	// 0x40
	FIS_REG_D2H	rfis;		// Register – Device to Host FIS
	UINT8_T         pad2[4];

	// 0x58
	//FIS_DEV_BITS	sdbfis;		// Set Device Bit FIS
	UINT16_T sdbfis;		// Set Device Bit FIS

	// 0x60
	UINT8_T         ufis[64];

	// 0xA0
	UINT8_T   	rsv[0x100 - 0xA0];
} HBA_FIS;

typedef struct tagHBA_CMD_HEADER
{
	// DW0
	UINT8_T  cfl : 5;		// Command FIS length in DWORDS, 2 ~ 16
	UINT8_T  a : 1;		// ATAPI
	UINT8_T  w : 1;		// Write, 1: H2D, 0: D2H
	UINT8_T  p : 1;		// Prefetchable

	UINT8_T  r : 1;		// Reset
	UINT8_T  b : 1;		// BIST
	UINT8_T  c : 1;		// Clear busy upon R_OK
	UINT8_T  rsv0 : 1;		// Reserved
	UINT8_T  pmp : 4;		// Port multiplier port

	UINT16_T prdtl;		// Physical region descriptor table length in entries

	// DW1
	volatile UINT32_T prdbc;		// Physical region descriptor byte count transferred

	// DW2, 3
	UINT32_T ctba;		// Command table descriptor base address
	UINT32_T ctbau;		// Command table descriptor base address upper 32 bits

	// DW4 - 7
	UINT32_T rsv1[4];	// Reserved
} HBA_CMD_HEADER;

typedef struct tagHBA_PRDT_ENTRY
{
	UINT32_T dba;		// Data base address
	UINT32_T dbau;		// Data base address upper 32 bits
	UINT32_T rsv0;		// Reserved

	// DW3
	UINT32_T dbc : 22;		// Byte count, 4M max
	UINT32_T rsv1 : 9;		// Reserved
	UINT32_T i : 1;		// Interrupt on completion
} HBA_PRDT_ENTRY;

typedef struct tagHBA_CMD_TBL
{
	// 0x00
	UINT8_T  cfis[64];	// Command FIS

	// 0x40
	UINT8_T  acmd[16];	// ATAPI command, 12 or 16 bytes

	// 0x50
	UINT8_T  rsv[48];	// Reserved

	// 0x80
	HBA_PRDT_ENTRY	prdt_entry[1];	// Physical region descriptor table entries, 0 ~ 65535
} HBA_CMD_TBL;


typedef struct tagAHCI_PORT
{
	HBA_PORT* hbaPort;
	UINT32_T* clbVirtualAddress;
	BOOL available;
	enum tagDRIVE_TYPE
	{
		DRIVE_TYPE_INVALID = -1,
		DRIVE_TYPE_SATA,
		DRIVE_TYPE_SATAPI,
	} driveType;
} AHCI_PORT;