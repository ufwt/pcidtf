/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (KMDF)
 * This file implements device object functions.
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

EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE EvtDeviceReleaseHardware;

VOID SetDeviceInit(__in PWDFDEVICE_INIT DeviceInit)
{
	WDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacks;

	PAGED_CODE();

	WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPowerEventCallbacks);
	PnpPowerEventCallbacks.EvtDevicePrepareHardware =
	    EvtDevicePrepareHardware;
	PnpPowerEventCallbacks.EvtDeviceReleaseHardware =
	    EvtDeviceReleaseHardware;

	WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit,
					       &PnpPowerEventCallbacks);
}

NTSTATUS PciDtfDeviceInit(__in WDFDEVICE Device)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	WDF_IO_QUEUE_CONFIG QueueConfig;
	WDFQUEUE Queue;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes;
	WDF_DMA_ENABLER_CONFIG DmaEnablerConfig;
	NTSTATUS Status = STATUS_SUCCESS;

	PAGED_CODE();

	__try {
		RtlZeroMemory(DeviceData, sizeof(DEVICE_DATA));

		Status = WdfDeviceCreateDeviceInterface(Device,
							&GUID_PCIDTF_DEVICE_INTERFACE_CLASS,
							NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfDeviceCreateDeviceInterface", Status);
			__leave;
		}

		WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig,
						       WdfIoQueueDispatchParallel);
		QueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

		Status = WdfIoQueueCreate(Device, &QueueConfig,
					  WDF_NO_OBJECT_ATTRIBUTES, &Queue);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfIoQueueCreate", Status);
			__leave;
		}

		WDF_OBJECT_ATTRIBUTES_INIT(&ObjectAttributes);
		ObjectAttributes.ParentObject = Device;
		Status = WdfCollectionCreate(&ObjectAttributes,
					     &DeviceData->RegSpaces);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfCollectionCreate", Status);
			__leave;
		}

		WDF_DMA_ENABLER_CONFIG_INIT(&DmaEnablerConfig,
					    WdfDmaProfileScatterGather,
					    (size_t) - 1);
		Status = WdfDmaEnablerCreate(Device, &DmaEnablerConfig,
					     WDF_NO_OBJECT_ATTRIBUTES,
					     &DeviceData->DmaEnabler);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfDmaEnablerCreate", Status);
			__leave;
		}

		Status = WdfCollectionCreate(&ObjectAttributes,
					     &DeviceData->CommonBuffers);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfCollectionCreate", Status);
			__leave;
		}
	}
	__finally {
	}
	return Status;
}

NTSTATUS
EvtDevicePrepareHardware(IN WDFDEVICE Device,
			 IN WDFCMRESLIST ResourcesRaw,
			 IN WDFCMRESLIST ResourcesTranslated)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	ULONG Index, Count;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDesc;
	NTSTATUS Status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(ResourcesRaw);

	__try {
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DEVICE, "Device 0x%p\n",
			  Device);

		Count = WdfCmResourceListGetCount(ResourcesTranslated);
		for (Index = 0; Index < Count; Index++) {
			ResourceDesc =
			    WdfCmResourceListGetDescriptor(ResourcesTranslated,
							   Index);
			if (ResourceDesc == NULL) {
				TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEVICE,
					  "WdfCmResourceListGetDescriptor failed\n");
				Status = STATUS_UNSUCCESSFUL;
				break;
			}

			TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DEVICE,
				  "Type=%s\n",
				  GetResourceTypeString(ResourceDesc->Type));
			TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DEVICE,
				  "ShareDisposition=%s\n",
				  GetResourceShareString
				  (ResourceDesc->ShareDisposition));

			switch (ResourceDesc->Type) {

			case CmResourceTypePort:
			case CmResourceTypeMemory:
				TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DEVICE,
					  "Start=0x%llX, Length=0x%X\n",
					  ResourceDesc->u.Memory.Start.QuadPart,
					  ResourceDesc->u.Memory.Length);
				Status = RegSpaceCreate(Device, ResourceDesc);
				if (!NT_SUCCESS(Status))
					__leave;
				break;

			default:
				break;
			}
		}

		Status =
		    WdfIoTargetQueryForInterface(WdfDeviceGetIoTarget(Device),
						 &GUID_BUS_INTERFACE_STANDARD,
						 (PINTERFACE) &
						 DeviceData->BusIntf,
						 sizeof(BUS_INTERFACE_STANDARD),
						 1, NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfIoTargetQueryForInterface", Status);
			__leave;
		}
	}
	__finally {
	}
	return Status;
}

NTSTATUS
EvtDeviceReleaseHardware(IN WDFDEVICE Device,
			 IN WDFCMRESLIST ResourcesTranslated)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	PVOID Context;
	NTSTATUS Status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(ResourcesTranslated);

	TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DEVICE, "Device 0x%p\n", Device);

	Context =
	    InterlockedExchangePointer(&DeviceData->BusIntf.Context, NULL);
	if (Context)
		DeviceData->BusIntf.InterfaceDereference(Context);

	return Status;
}
