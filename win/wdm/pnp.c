/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (WDM)
 * This file implements plug and play functions.
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

NTSTATUS PciDtfStartDevice(IN PDEVICE_DATA DeviceData,
			   IN PIO_STACK_LOCATION IrpStack);

NTSTATUS DriverDispatchPnp(__in PDEVICE_OBJECT DeviceObject, __inout PIRP Irp)
{
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION IrpStack;
	int Index;
	NTSTATUS Status = STATUS_SUCCESS;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);

	switch (IrpStack->MinorFunction) {

	case IRP_MN_START_DEVICE:
		Status = WdmSyncSendIrp(DeviceData->NextDeviceObject, Irp);
		if (NT_SUCCESS(Status)) {
			Status = PciDtfStartDevice(DeviceData, IrpStack);
		} else {
			TRACE_ERR("WdmSyncSendIrp", Status);
		}
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;

	case IRP_MN_REMOVE_DEVICE:
		IoSetDeviceInterfaceState(&DeviceData->SymbolicLinkName, FALSE);
		if (DeviceData->BusIntf.Context)
			DeviceData->BusIntf.
			    InterfaceDereference(DeviceData->BusIntf.Context);
		for (Index = 0; Index < DeviceData->RegSpaceCount; Index++)
			PciDtfRegSpaceCleanup(DeviceData->RegSpaceData + Index);
		xpcf_collection_cleanup(&DeviceData->CommonBuffers,
					PciDtfCleanupCommonBuffer);
		if (DeviceData->DmaAdapter)
			DeviceData->DmaAdapter->
			    DmaOperations->PutDmaAdapter(DeviceData->
							 DmaAdapter);
		IoSkipCurrentIrpStackLocation(Irp);
		Status = IoCallDriver(DeviceData->NextDeviceObject, Irp);
		IoDetachDevice(DeviceData->NextDeviceObject);

		RtlFreeUnicodeString(&DeviceData->SymbolicLinkName);
		IoDeleteDevice(DeviceObject);

		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_PNP,
			  "Device 0x%p removed\n", DeviceObject);
		break;

	default:
		IoSkipCurrentIrpStackLocation(Irp);
		Status = IoCallDriver(DeviceData->NextDeviceObject, Irp);
		break;
	}
	return Status;
}

NTSTATUS PciDtfStartDevice(IN PDEVICE_DATA DeviceData,
			   IN PIO_STACK_LOCATION IrpStack)
{
	PCM_RESOURCE_LIST ResourceList;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDesc;
	DEVICE_DESCRIPTION DeviceDesc;
	ULONG Index, Count, NumOfMapRegs;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		ResourceList =
		    IrpStack->Parameters.
		    StartDevice.AllocatedResourcesTranslated;
		if (ResourceList->Count == 0) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		Count = ResourceList->List[0].PartialResourceList.Count;
		ResourceDesc =
		    ResourceList->List[0].
		    PartialResourceList.PartialDescriptors;
		for (Index = 0; Index < Count; Index++, ResourceDesc++) {
			switch (ResourceDesc->Type) {

			case CmResourceTypePort:
			case CmResourceTypeMemory:
				if (DeviceData->RegSpaceCount < MAX_REG_SPACES) {
					Status =
					    PciDtfRegSpaceInit
					    (DeviceData->RegSpaceData +
					     DeviceData->RegSpaceCount,
					     ResourceDesc);
					if (!NT_SUCCESS(Status)) {
						TRACE_ERR("PciDtfRegSpaceInit",
							  Status);
						__leave;
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
		    IoGetDmaAdapter(DeviceData->PhysicalDeviceObject,
				    &DeviceDesc, &NumOfMapRegs);
		if (DeviceData->DmaAdapter == NULL) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			TRACE_ERR("IoGetDmaAdapter", Status);
			__leave;
		}
		// Initialize bus interface
		Status = WdmQueryInterface(DeviceData->PhysicalDeviceObject,
					   &GUID_BUS_INTERFACE_STANDARD,
					   sizeof(BUS_INTERFACE_STANDARD), 1,
					   (PINTERFACE) & DeviceData->BusIntf,
					   NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdmQueryInterface", Status);
			__leave;
		}
		// Enable device interface
		Status =
		    IoSetDeviceInterfaceState(&DeviceData->SymbolicLinkName,
					      TRUE);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("IoSetDeviceInterfaceState", Status);
			__leave;
		}
	}
	__finally {
	}
	return Status;
}
