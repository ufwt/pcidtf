/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (WDM)
 * This file implements device level functions.
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciDtfDeviceOpen)
#pragma alloc_text(PAGE, PciDtfDeviceStart)
#pragma alloc_text(PAGE, PciDtfDeviceRemove)
#endif

NTSTATUS PciDtfDeviceOpen(__in BASE_DEVICE * Device, __out BASE_FILE ** ppFile)
{
	BASE_FILE *File;
	BASE_FILE_PARAMS Params;
	NTSTATUS Status = STATUS_SUCCESS;

	PAGED_CODE();

	RtlZeroMemory(&Params, sizeof(Params));
	Params.Ioctl = PciDtfFileIoctl;

	Status = BaseFileCreate(Device, &Params, &File);
	if (!NT_SUCCESS(Status)) {
		TRACE_ERR("BaseFileCreate", Status);
	} else {
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DEVICE,
			  "Device 0x%p opened (file 0x%p)\n", Device, File);
		*ppFile = File;
	}
	return Status;
}

NTSTATUS PciDtfDeviceStart(__in BASE_DEVICE * Device,
			   __in PIO_STACK_LOCATION IrpStack)
{
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	PDEVICE_OBJECT PhysicalDeviceObject =
	    BaseDeviceGetPhysicalDeviceObject(Device);
	PCM_RESOURCE_LIST ResourceList;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDesc;
	DEVICE_DESCRIPTION DeviceDesc;
	ULONG Index, Count, NumOfMapRegs;
	NTSTATUS Status = STATUS_SUCCESS;

	PAGED_CODE();

	ResourceList =
	    IrpStack->Parameters.StartDevice.AllocatedResourcesTranslated;
	if (ResourceList->Count == 0) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	Count = ResourceList->List[0].PartialResourceList.Count;
	ResourceDesc =
	    ResourceList->List[0].PartialResourceList.PartialDescriptors;
	for (Index = 0; Index < Count; Index++, ResourceDesc++) {
		switch (ResourceDesc->Type) {

		case CmResourceTypePort:
		case CmResourceTypeMemory:
			if (DeviceData->RegSpaceCount < MAX_REG_SPACES) {
				Status =
				    PciDtfRegSpaceInit
				    (DeviceData->RegSpaceData +
				     DeviceData->RegSpaceCount, ResourceDesc);
				if (!NT_SUCCESS(Status)) {
					TRACE_ERR("PciDtfRegSpaceInit", Status);
					return Status;
				}
				DeviceData->RegSpaceCount++;
			}
			break;

		default:
			break;
		}
	}

	// Initialize DMA adapter
	RtlZeroMemory(&DeviceDesc, sizeof(DEVICE_DESCRIPTION));
	DeviceDesc.Version = DEVICE_DESCRIPTION_VERSION;
	DeviceDesc.Master = TRUE;
	DeviceDesc.ScatterGather = TRUE;
	DeviceDesc.Dma32BitAddresses = TRUE;
	DeviceDesc.MaximumLength = (ULONG) - 1;
	DeviceData->DmaAdapter =
	    IoGetDmaAdapter(PhysicalDeviceObject, &DeviceDesc, &NumOfMapRegs);
	if (DeviceData->DmaAdapter == NULL) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		TRACE_ERR("IoGetDmaAdapter", Status);
		return Status;
	}
	DeviceData->DmaOperations = DeviceData->DmaAdapter->DmaOperations;

	// Initialize bus interface
	Status = WdmQueryInterface(PhysicalDeviceObject,
				   &GUID_BUS_INTERFACE_STANDARD,
				   sizeof(BUS_INTERFACE_STANDARD), 1,
				   (PINTERFACE) & DeviceData->BusIntf, NULL);
	if (!NT_SUCCESS(Status)) {
		TRACE_ERR("WdmQueryInterface", Status);
		return Status;
	}
	return Status;
}

void PciDtfDeviceRemove(__in BASE_DEVICE * Device)
{
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	PVOID Context = DeviceData->BusIntf.Context;
	PDMA_OPERATIONS DmaOps = DeviceData->DmaOperations;
	int Index;

	PAGED_CODE();

	if (Context)
		DeviceData->BusIntf.InterfaceDereference(Context);
	for (Index = 0; Index < DeviceData->RegSpaceCount; Index++)
		PciDtfRegSpaceCleanup(DeviceData->RegSpaceData + Index);
	xpcf_collection_cleanup(&DeviceData->CommonBuffers,
				PciDtfCleanupCommonBuffer);
	if (DeviceData->DmaAdapter)
		DmaOps->PutDmaAdapter(DeviceData->DmaAdapter);
}
