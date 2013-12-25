/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (WDM)
 * This file implements driver level functions.
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

#include "precomp.h"
#include "pcidtf_wdm.h"

DECLARE_MOD_INFO("pcidtf_wdm", TRACE_LEVEL_INFO,
		 TRACE_FLAG_DEFAULT | TRACE_FLAG_DRIVER | TRACE_FLAG_PNP |
		 TRACE_FLAG_POWER | TRACE_FLAG_WMI | TRACE_FLAG_IOCTL |
		 TRACE_FLAG_DMA);

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
DRIVER_ADD_DEVICE DriverAddDevice;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, DriverAddDevice)
#endif

NTSTATUS DriverEntry(__in PDRIVER_OBJECT DriverObject,
		     __in PUNICODE_STRING RegistryPath)
{
	NTSTATUS Status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = DriverAddDevice;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DriverDispatchPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = DriverDispatchPower;
	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] =
	    DriverDispatchSystemControl;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverDispatchOpenClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverDispatchOpenClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] =
	    DriverDispatchDeviceControl;

	TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DRIVER, "Driver initialized\n");

	return Status;
}

VOID DriverUnload(__in PDRIVER_OBJECT DriverObject)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(DriverObject);

	TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DRIVER, "Driver unloaded\n");
}

NTSTATUS DriverAddDevice(__in PDRIVER_OBJECT DriverObject,
			 __in PDEVICE_OBJECT PhysicalDeviceObject)
{
	PDEVICE_OBJECT DeviceObject = NULL;
	PDEVICE_DATA DeviceData = NULL;
	NTSTATUS Status = STATUS_SUCCESS;

	PAGED_CODE();

	__try {
		Status = IoCreateDevice(DriverObject, sizeof(DEVICE_DATA),
					NULL, FILE_DEVICE_UNKNOWN, 0, FALSE,
					&DeviceObject);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("IoCreateDevice", Status);
			__leave;
		}

		DeviceData = (PDEVICE_DATA) DeviceObject->DeviceExtension;
		RtlZeroMemory(DeviceData, sizeof(DEVICE_DATA));

		xpcf_collection_init(&DeviceData->CommonBuffers,
				     DEFAULT_NUM_COMMON_BUFS, 1);
		DeviceData->CommonBuffers.ctx = DeviceData;

		Status = IoRegisterDeviceInterface(PhysicalDeviceObject,
						   &GUID_PCIDTF_DEVICE_INTERFACE_CLASS,
						   NULL,
						   &DeviceData->SymbolicLinkName);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("IoRegisterDeviceInterface", Status);
			__leave;
		}

		DeviceData->NextDeviceObject =
		    IoAttachDeviceToDeviceStack(DeviceObject,
						PhysicalDeviceObject);
		if (DeviceData->NextDeviceObject == NULL) {
			Status = STATUS_NO_SUCH_DEVICE;
			TRACE_ERR("IoAttachDeviceToDeviceStack", Status);
			__leave;
		}
		// Save PDO
		DeviceData->PhysicalDeviceObject = PhysicalDeviceObject;

		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_PNP,
			  "Device 0x%p added\n", DeviceObject);
	}
	__finally {
		if (!NT_SUCCESS(Status) && DeviceObject != NULL) {
			xpcf_collection_cleanup(&DeviceData->CommonBuffers,
						PciDtfCleanupCommonBuffer);
			IoDeleteDevice(DeviceObject);
		}
	}
	return Status;
}
