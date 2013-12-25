/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (KMDF)
 * This file implements driver object functions.
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
#include "pcidtf_kmdf.h"

DECLARE_MOD_INFO("pcidtf_kmdf", TRACE_LEVEL_INFO,
		 TRACE_FLAG_DEFAULT | TRACE_FLAG_DRIVER | TRACE_FLAG_DEVICE |
		 TRACE_FLAG_QUEUE | TRACE_FLAG_REG_SPACE);

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;
EVT_WDF_DRIVER_UNLOAD EvtDriverUnload;

NTSTATUS
DriverEntry(__in PDRIVER_OBJECT DriverObject, __in PUNICODE_STRING RegistryPath)
{
	WDF_DRIVER_CONFIG DriverConfig;
	NTSTATUS Status = STATUS_SUCCESS;

	WDF_DRIVER_CONFIG_INIT(&DriverConfig, EvtDriverDeviceAdd);
	DriverConfig.EvtDriverUnload = EvtDriverUnload;

	Status = WdfDriverCreate(DriverObject, RegistryPath,
				 WDF_NO_OBJECT_ATTRIBUTES, &DriverConfig,
				 WDF_NO_HANDLE);
	if (NT_SUCCESS(Status)) {
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DRIVER,
			  "Driver initialized\n");
	} else {
		TRACE_ERR("WdfDriverCreate", Status);
	}
	return Status;
}

NTSTATUS EvtDriverDeviceAdd(IN WDFDRIVER Driver, IN PWDFDEVICE_INIT DeviceInit)
{
	WDFDEVICE Device = NULL;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS Status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(Driver);

	__try {
		SetDeviceInit(DeviceInit);

		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ObjectAttributes,
							DEVICE_DATA);

		Status = WdfDeviceCreate(&DeviceInit, &ObjectAttributes,
					 &Device);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfDeviceCreate", Status);
			__leave;
		}

		Status = PciDtfDeviceInit(Device);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("DeviceInit", Status);
			__leave;
		}
	}
	__finally {
		if (!NT_SUCCESS(Status) && Device != NULL)
			WdfObjectDelete(Device);
	}
	return Status;
}

VOID EvtDriverUnload(IN WDFDRIVER Driver)
{
	UNREFERENCED_PARAMETER(Driver);

	TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DRIVER, "Driver unloaded\n");
}
