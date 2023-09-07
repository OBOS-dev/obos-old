/*
	driver/storage/nvme/nvme.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

typedef volatile struct tagNVME_MEM
{
	volatile struct
	{
		UINT16_T mqes;
		BOOL cqr : 1;
		UINT8_T ams : 2;
		UINT8_T resv1 : 5;
		UINT8_T timeout : 8;
		UINT8_T dstrd : 4;
		BOOL nssrs : 1;
		UINT8_T css : 8;
		BOOL bps : 1;
		UINT8_T cps : 2;
		UINT8_T mpsmin : 4;
		BOOL pmrs : 1;
		BOOL nsss : 1;
		UINT8_T crms : 2;
		UINT8_T resv2 : 4;
	} cap;
	const enum {
		NVME_VERSION_1_0_0 = 0x00010000,
		NVME_VERSION_1_1_0 = 0x00010100,
		NVME_VERSION_1_2_0 = 0x00010200,
		NVME_VERSION_1_2_1 = 0x00010201,
		NVME_VERSION_1_3_0 = 0x00010300,
		NVME_VERSION_1_4_0 = 0x00010400,
		NVME_VERSION_2_0_0 = 0x00020000,
	} version;
	UINT32_T intms;
	UINT32_T intmc;
	volatile struct {
		BOOL enable : 1;
		const UINT8_T resv1 : 3;
		UINT8_T css : 2;
		UINT8_T mps : 4;
		UINT8_T ams : 3;
		UINT8_T shn : 2;
		UINT8_T iosqes : 4;
		UINT8_T iocqes : 4;
		BOOL crime : 1;
		const UINT8_T resv2 : 7;
	} cc;
	volatile struct {
		const BOOL rdy : 1;
		const BOOL cfs : 1;
		const UINT8_T shst : 2;
		BOOL nssro : 1;
		const BOOL pp : 1;
		const BOOL st : 1;
		const UINT32_T resv1 : 25;
	} csts;
	UINT32_T nssr;
	volatile struct {
		UINT16_T asqa : 12;
		const UINT8_T resv1 : 4;
		UINT16_T acqs : 12;
		const UINT8_T resv2 : 4;
	} aqa;
	volatile struct {
		const UINT16_T resv1 : 12;
		UINT64_T asqb : 52;
	} asq;
	volatile struct {
		const UINT16_T resv1 : 12;
		UINT64_T acqb : 52;
	} acq;
	volatile struct {
		const UINT8_T bir : 3;
		const BOOL cqmms : 1;
		const BOOL cqpds : 1;
		const BOOL cdpmls : 1;
		const BOOL cdpcils : 1;
		const BOOL cdmmms : 1;
		const BOOL cqda : 1;
		const UINT8_T resv1 : 4;
		const UINT32_T ofst : 20;
	} cmbloc;
	volatile struct {
		const BOOL sqs : 1;
		const BOOL cqs : 1;
		const BOOL lists : 1;
		const BOOL rds : 1;
		const BOOL wds : 1;
		const UINT8_T resv1 : 3;
		const UINT8_T szu : 4;
		const UINT32_T sz : 20;
	} cmbsz;
	volatile struct {
		const UINT16_T bpsz : 15;
		const UINT16_T resv1 : 9;
		const UINT8_T brs : 2;
		const UINT8_T resv2 : 5;
		const BOOL abpid : 1;
	} bpinfo;
	volatile struct {
		UINT16_T bprsz : 10;
		UINT32_T bprof : 20;
		const BOOL resv1 : 1;
		BOOL bpid : 1;
	} bprsel;
	volatile struct {
		const UINT16_T resv1 : 12;
		UINT64_T bmbba : 52;
	} bpmbl;
	volatile struct {
		BOOL cre : 1;
		BOOL cmse : 1;
		const UINT16_T resv1 : 10;
		UINT64_T cba : 52;
	} cmbmsc;
	volatile struct {
		const BOOL cbai : 1;
		const UINT32_T resv1 : 31;
	} cmbsts;
	volatile struct {
		const UINT8_T cmbszu : 4;
		const BOOL readBybassBehaviour : 1;
		const UINT8_T resv1 : 3;
		const UINT32_T cmbwbz : 24;
	} cmbebs;
	volatile struct {
		const UINT8_T cmbswtu : 4;
		const UINT8_T resv : 4;
		const UINT32_T cmbswtv : 24;
	} cmbswtp;
	UINT32_T nssd;
	volatile struct {
		const UINT16_T crwmt;
		const UINT16_T crimt;
	} crto;
	volatile struct {
		const UINT8_T resv1 : 3;
		const BOOL rds : 1;
		const BOOL wds : 1;
		const UINT8_T bir : 3;
		const UINT8_T pmrtu: 2;
		const UINT8_T pmrwbm: 4;
		const UINT8_T resv2 : 3;
		const UINT8_T pmrto : 8;
		const BOOL cmss : 1;
		const UINT8_T resv3 : 7;
	} pmrcap;
	volatile struct {
		BOOL enable : 1;
		const UINT32_T resv1 : 31;
	} pmrctl;
	volatile struct {
		const UINT8_T err;
		const BOOL nrdy : 1;
		const UINT8_T hsts : 3;
		const BOOL cbai : 1;
		const UINT32_T resv1 : 19;
	} pmrsts;
	volatile struct {
		const UINT8_T pmrszu : 4;
		const BOOL readBypassBehaviour : 1;
		const UINT8_T resv1 : 3;
		const UINT32_T pmrwbz : 24;
	} pmrebs;
	volatile struct {
		const UINT8_T pmrswtu : 4;
		const UINT8_T resv1 : 4;
		const UINT32_T pmrswtv : 24;
	} pmrswtp;
	volatile struct {
		const BOOL resv1 : 1;
		BOOL cmse : 1;
		const UINT16_T resv2 : 10;
		UINT32_T cba : 20;
	} pmrmscl;
	UINT32_T pmrmscu;
} NVME_MEM;