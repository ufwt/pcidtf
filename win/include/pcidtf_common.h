/*
 * PCI Device Test Framework
 * This file defines structure type and function prototypes that are
 * common for Windows kernel-mode drivers (WDM and KMDF).
 *
 * Copyright (C) 2013 Hiromitsu Sakamoto
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

#ifndef _PCI_DTF_COMMON_H
#define _PCI_DTF_COMMON_H

#include "pcidtf_ioctl.h"

//
// Register space context data
//

typedef struct _REG_SPACE_DATA {
	CM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDesc;
	PVOID MappedIoSpace;
} REG_SPACE_DATA, *PREG_SPACE_DATA;

//
// Function prototypes
//

// PCI configuration space access functions
NTSTATUS PciDtfConfigRead(IN PBUS_INTERFACE_STANDARD BusIntf, IN ULONG Offset,
			  IN ULONG Length, OUT UINT32 * Value);
NTSTATUS PciDtfConfigWrite(IN PBUS_INTERFACE_STANDARD BusIntf, IN ULONG Offset,
			   IN ULONG Length, IN UINT32 Value);

// I/O register space access functions
NTSTATUS PciDtfRegSpaceInit(IN PREG_SPACE_DATA RegSpaceData,
			    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDesc);
VOID PciDtfRegSpaceCleanup(IN PREG_SPACE_DATA RegSpaceData);
NTSTATUS PciDtfRegSpaceRead(IN PREG_SPACE_DATA RegSpaceData, IN ULONG Offset,
			    IN ULONG Length, OUT PULONGLONG Value);
NTSTATUS PciDtfRegSpaceWrite(IN PREG_SPACE_DATA RegSpaceData, IN ULONG Offset,
			     IN ULONG Length, IN ULONGLONG Value);

// Common buffer access function
NTSTATUS PciDtfDmaReadWrite(IN PCIDTF_DMA_DATA * ReqData,
			    IN PVOID VirtualAddress, IN ULONG Length,
			    IN BOOLEAN Read);

#endif
