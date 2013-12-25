/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (KMDF)
 * This file defines macros, structure types and function prototypes
 * of Windows KMDF-based driver.
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

#ifndef _PCIDTF_KMDF_H
#define _PCIDTF_KMDF_H

#include "pcidtf_common.h"

// Trace logging flags
#define TRACE_FLAG_DRIVER       TRACE_FLAG(1)
#define TRACE_FLAG_DEVICE       TRACE_FLAG(2)
#define TRACE_FLAG_QUEUE        TRACE_FLAG(3)
#define TRACE_FLAG_REG_SPACE    TRACE_FLAG(4)

//
// Device context data
//

typedef struct _DEVICE_DATA {
	BUS_INTERFACE_STANDARD BusIntf;
	WDFCOLLECTION RegSpaces;
	WDFDMAENABLER DmaEnabler;
	WDFCOLLECTION CommonBuffers;
} DEVICE_DATA, *PDEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_DATA, GetDeviceData);

//
// Register space context data
//

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REG_SPACE_DATA, GetRegSpaceData);

//
// Common buffer context data
//

typedef struct _COMMON_BUFFER_DATA {
	int ID;
} COMMON_BUFFER_DATA, *PCOMMON_BUFFER_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(COMMON_BUFFER_DATA, GetCommonBufferData);

//
// External function prototypes
//

// device.c
VOID SetDeviceInit(__in PWDFDEVICE_INIT DeviceInit);
NTSTATUS PciDtfDeviceInit(__in WDFDEVICE Device);

// queue.c
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

// regspace.c
NTSTATUS RegSpaceCreate(__in WDFDEVICE Device,
			__in PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDesc);
NTSTATUS PciDtfRegSpaceGet(IN WDFDEVICE Device, IN ULONG Index,
			   OUT WDFOBJECT * ppRegSpace);

#endif
