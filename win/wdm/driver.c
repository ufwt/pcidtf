/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (WDM)
 * This file implements driver level functions.
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

#include "precomp.h"
#include "pcidtf_wdm.h"

DECLARE_MOD_INFO("pcidtf_wdm", TRACE_LEVEL_INFO,
		 TRACE_FLAG_DEFAULT | TRACE_FLAG_DRIVER | TRACE_FLAG_DEVICE |
		 TRACE_FLAG_IOCTL | TRACE_FLAG_DMA);

DRIVER_INITIALIZE DriverEntry;
BASE_DRIVER_ADD_DEVICE PciDtfDriverAddDevice;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PciDtfDriverAddDevice)
#endif

NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject,
		     __in PUNICODE_STRING RegistryPath)
{
	BASE_DRIVER *Driver;
	BASE_DRIVER_PARAMS Params;
	NTSTATUS Status;

	RtlZeroMemory(&Params, sizeof(Params));
	Params.AddDevice = PciDtfDriverAddDevice;

	Status = BaseDriverCreate(DriverObject, RegistryPath, &Params, &Driver);
	if (!NT_SUCCESS(Status)) {
		TRACE_ERR("BaseDriverCreate", Status);
	} else {
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DRIVER,
			  "Driver initialized\n");
	}
	return Status;
}

NTSTATUS PciDtfDriverAddDevice(__in BASE_DRIVER * Driver,
			       __in PDEVICE_OBJECT
			       PhysicalDeviceObject,
			       __out BASE_DEVICE ** ppDevice)
{
	BASE_DEVICE *Device;
	BASE_DEVICE_PARAMS Params;
	PDEVICE_DATA DeviceData;
	NTSTATUS Status;

	RtlZeroMemory(&Params, sizeof(Params));
	Params.PrivateAreaSize = sizeof(DEVICE_DATA);
	Params.PhysicalDeviceObject = PhysicalDeviceObject;
	Params.InterfaceClassGuid = &GUID_PCIDTF_DEVICE_INTERFACE_CLASS;
	Params.Open = PciDtfDeviceOpen;
	Params.Start = PciDtfDeviceStart;
	Params.Remove = PciDtfDeviceRemove;

	Status = BaseDeviceCreate(Driver, &Params, &Device);
	if (!NT_SUCCESS(Status)) {
		TRACE_ERR("BaseDeviceCreate", Status);
	} else {
		DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
		xpcf_collection_init(&DeviceData->CommonBuffers,
				     DEFAULT_NUM_COMMON_BUFS, 1);
		DeviceData->CommonBuffers.ctx = DeviceData;
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DRIVER,
			  "Device 0x%p added\n", Device);
		*ppDevice = Device;
	}
	return Status;
}
