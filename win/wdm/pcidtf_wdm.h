/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (WDM)
 * This file defines macros, structure types and function prototypes
 * of Windows WDM-based driver.
 *
 * Copyright (C) 2013-2014 Hiromitsu Sakamoto
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

#ifndef _PCIDTF_WDM_H
#define _PCIDTF_WDM_H

#include "pcidtf_common.h"

#define TRACE_FLAG_DRIVER   TRACE_FLAG(1)
#define TRACE_FLAG_DEVICE   TRACE_FLAG(2)
#define TRACE_FLAG_IOCTL    TRACE_FLAG(3)
#define TRACE_FLAG_DMA      TRACE_FLAG(4)

#define PCIDTF_POOL_TAG 'FTDP'

#define MAX_REG_SPACES  6

#define DEFAULT_NUM_COMMON_BUFS 8

typedef struct _DEVICE_DATA {
	BUS_INTERFACE_STANDARD BusIntf;
	REG_SPACE_DATA RegSpaceData[MAX_REG_SPACES];
	int RegSpaceCount;
	PDMA_ADAPTER DmaAdapter;
	PDMA_OPERATIONS DmaOperations;
	XPCF_COLLECTION CommonBuffers;
} DEVICE_DATA, *PDEVICE_DATA;

typedef struct _COMMON_BUFFER_DATA {
	PVOID VirtualAddress;
	LARGE_INTEGER PhysicalAddress;
	ULONG Length;
} COMMON_BUFFER_DATA, *PCOMMON_BUFFER_DATA;

// device.c
BASE_DEVICE_OPEN PciDtfDeviceOpen;
BASE_DEVICE_START PciDtfDeviceStart;
BASE_DEVICE_REMOVE PciDtfDeviceRemove;

// ioctl.c
BASE_FILE_IOCTL PciDtfFileIoctl;

// dma.c
NTSTATUS PciDtfDmaCreate(IN PDEVICE_DATA DeviceData, IN ULONG Length,
			 OUT PCOMMON_BUFFER_DATA * ppCommonBufferData,
			 OUT int *id);
XPCF_COLLECTION_REMOVE_HANDLER PciDtfCleanupCommonBuffer;

#endif
