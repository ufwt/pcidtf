/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (WDM)
 * This file implements I/O control functions.
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
#include "pcidtf_ioctl.h"

//
// Local function prototypes
//

NTSTATUS PciDtfGetInfo(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
		       IN PIO_STACK_LOCATION IrpStack);
NTSTATUS PciDtfReadWriteCfg(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read);
NTSTATUS PciDtfGetReg(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
		      IN PIO_STACK_LOCATION IrpStack);
NTSTATUS PciDtfAllocDma(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
			IN PIO_STACK_LOCATION IrpStack);
NTSTATUS PciDtfFreeDma(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
		       IN PIO_STACK_LOCATION IrpStack);
NTSTATUS PciDtfReadWriteReg(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read);
NTSTATUS PciDtfReadWriteDma(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read);
NTSTATUS PciDtfGetDma(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
		      IN PIO_STACK_LOCATION IrpStack);

NTSTATUS DriverDispatchOpenClose(__in PDEVICE_OBJECT DeviceObject,
				 __inout PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS DriverDispatchDeviceControl(__in PDEVICE_OBJECT DeviceObject,
				     __inout PIRP Irp)
{
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) DeviceObject->DeviceExtension;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS Status;

	switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

	case IOCTL_PCIDTF_GET_INFO:
		Status = PciDtfGetInfo(DeviceData, Irp, IrpStack);
		break;

	case IOCTL_PCIDTF_READ_CFG:
		Status = PciDtfReadWriteCfg(DeviceData, Irp, IrpStack, TRUE);
		break;

	case IOCTL_PCIDTF_WRITE_CFG:
		Status = PciDtfReadWriteCfg(DeviceData, Irp, IrpStack, FALSE);
		break;

	case IOCTL_PCIDTF_GET_REG:
		Status = PciDtfGetReg(DeviceData, Irp, IrpStack);
		break;

	case IOCTL_PCIDTF_READ_REG:
		Status = PciDtfReadWriteReg(DeviceData, Irp, IrpStack, TRUE);
		break;

	case IOCTL_PCIDTF_WRITE_REG:
		Status = PciDtfReadWriteReg(DeviceData, Irp, IrpStack, FALSE);
		break;

	case IOCTL_PCIDTF_ALLOC_DMA:
		Status = PciDtfAllocDma(DeviceData, Irp, IrpStack);
		break;

	case IOCTL_PCIDTF_FREE_DMA:
		Status = PciDtfFreeDma(DeviceData, Irp, IrpStack);
		break;

	case IOCTL_PCIDTF_READ_DMA:
		Status = PciDtfReadWriteDma(DeviceData, Irp, IrpStack, TRUE);
		break;

	case IOCTL_PCIDTF_WRITE_DMA:
		Status = PciDtfReadWriteDma(DeviceData, Irp, IrpStack, FALSE);
		break;

	case IOCTL_PCIDTF_GET_DMA_INFO:
		Status = PciDtfGetDma(DeviceData, Irp, IrpStack);
		break;

	default:
		TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
			  "Unsupported I/O control code 0x%X\n",
			  IrpStack->Parameters.DeviceIoControl.IoControlCode);
		Status = STATUS_INVALID_DEVICE_REQUEST;
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		break;
	}
	return Status;
}

// Implement local functions

NTSTATUS PciDtfGetInfo(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
		       IN PIO_STACK_LOCATION IrpStack)
{
	PCIDTF_DEV_INFO *ReqData;
	ULONG Value, Length;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength !=
		    sizeof(PCIDTF_DEV_INFO)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_DEV_INFO *) Irp->AssociatedIrp.SystemBuffer;

		Status = IoGetDeviceProperty(DeviceData->PhysicalDeviceObject,
					     DevicePropertyBusNumber,
					     sizeof(Value), &Value, &Length);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR
			    ("IoGetDeviceProperty(DevicePropertyBusNumber)",
			     Status);
			__leave;
		}
		ReqData->bus = (UINT8) Value;

		Status = IoGetDeviceProperty(DeviceData->PhysicalDeviceObject,
					     DevicePropertyAddress,
					     sizeof(Value), &Value, &Length);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR
			    ("IoGetDeviceProperty(DevicePropertyAddress)",
			     Status);
			__leave;
		}
		ReqData->devfn = (UINT8) Value;
		ReqData->reg_count = DeviceData->RegSpaceCount;

		Irp->IoStatus.Information = sizeof(PCIDTF_DEV_INFO);
	}
	__finally {
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

NTSTATUS PciDtfReadWriteCfg(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read)
{
	PCIDTF_CFG_DATA *ReqData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
		    sizeof(PCIDTF_CFG_DATA)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_CFG_DATA *) Irp->AssociatedIrp.SystemBuffer;
		if (Read) {
			if (IrpStack->Parameters.
			    DeviceIoControl.OutputBufferLength !=
			    sizeof(PCIDTF_CFG_DATA)) {
				Status = STATUS_INVALID_BUFFER_SIZE;
				__leave;
			}
			Status = PciDtfConfigRead(&DeviceData->BusIntf,
						  ReqData->off, ReqData->len,
						  &ReqData->val);
			if (!NT_SUCCESS(Status)) {
				__leave;
			}
			Irp->IoStatus.Information = sizeof(PCIDTF_CFG_DATA);
		} else {
			Status = PciDtfConfigWrite(&DeviceData->BusIntf,
						   ReqData->off, ReqData->len,
						   ReqData->val);
		}
	}
	__finally {
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

NTSTATUS PciDtfGetReg(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
		      IN PIO_STACK_LOCATION IrpStack)
{
	PCIDTF_REG_INFO *ReqData;
	PREG_SPACE_DATA RegSpaceData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
		    sizeof(PCIDTF_REG_INFO)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength !=
		    sizeof(PCIDTF_REG_INFO)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_REG_INFO *) Irp->AssociatedIrp.SystemBuffer;
		if (ReqData->bar < 0
		    || ReqData->bar >= DeviceData->RegSpaceCount) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		RegSpaceData = DeviceData->RegSpaceData + ReqData->bar;
		if (RegSpaceData->ResourceDesc.Type == CmResourceTypePort) {
			ReqData->addr =
			    RegSpaceData->ResourceDesc.u.Port.Start.QuadPart;
			ReqData->len = RegSpaceData->ResourceDesc.u.Port.Length;
		} else {
			ReqData->addr =
			    RegSpaceData->ResourceDesc.u.Memory.Start.QuadPart;
			ReqData->len =
			    RegSpaceData->ResourceDesc.u.Memory.Length;
		}
		Irp->IoStatus.Information = sizeof(PCIDTF_REG_INFO);
	}
	__finally {
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

NTSTATUS PciDtfReadWriteReg(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read)
{
	PCIDTF_REG_DATA *ReqData;
	PREG_SPACE_DATA RegSpaceData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
		    sizeof(PCIDTF_REG_DATA)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_REG_DATA *) Irp->AssociatedIrp.SystemBuffer;
		if (ReqData->bar < 0
		    || ReqData->bar >= DeviceData->RegSpaceCount) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		RegSpaceData = DeviceData->RegSpaceData + ReqData->bar;
		if (Read) {
			if (IrpStack->Parameters.
			    DeviceIoControl.OutputBufferLength !=
			    sizeof(PCIDTF_REG_DATA)) {
				Status = STATUS_INVALID_BUFFER_SIZE;
				__leave;
			}
			Status = PciDtfRegSpaceRead(RegSpaceData,
						    ReqData->off, ReqData->len,
						    &ReqData->val);
			if (!NT_SUCCESS(Status)) {
				__leave;
			}
			Irp->IoStatus.Information = sizeof(PCIDTF_REG_DATA);
		} else {
			Status = PciDtfRegSpaceWrite(RegSpaceData,
						     ReqData->off, ReqData->len,
						     ReqData->val);
		}
	}
	__finally {
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

NTSTATUS PciDtfAllocDma(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
			IN PIO_STACK_LOCATION IrpStack)
{
	PCIDTF_DMA_INFO *ReqData;
	PCOMMON_BUFFER_DATA CommonBufferData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
		    sizeof(PCIDTF_DMA_INFO)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength !=
		    sizeof(PCIDTF_DMA_INFO)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_DMA_INFO *) Irp->AssociatedIrp.SystemBuffer;
		Status = PciDtfDmaCreate(DeviceData, ReqData->len,
					 &CommonBufferData, &ReqData->id);
		if (!NT_SUCCESS(Status)) {
			__leave;
		}
		ReqData->addr = CommonBufferData->PhysicalAddress.QuadPart;
		Irp->IoStatus.Information = sizeof(PCIDTF_DMA_INFO);
	}
	__finally {
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

NTSTATUS PciDtfFreeDma(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
		       IN PIO_STACK_LOCATION IrpStack)
{
	int id;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
		    sizeof(int)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		id = *(int *)Irp->AssociatedIrp.SystemBuffer;
		if (xpcf_collection_remove
		    (&DeviceData->CommonBuffers, id,
		     PciDtfCleanupCommonBuffer)) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
	}
	__finally {
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

NTSTATUS PciDtfReadWriteDma(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read)
{
	PCIDTF_DMA_DATA *ReqData;
	PCOMMON_BUFFER_DATA CommonBufferData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
		    sizeof(PCIDTF_DMA_DATA)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_DMA_DATA *) Irp->AssociatedIrp.SystemBuffer;
		CommonBufferData = (PCOMMON_BUFFER_DATA)
		    xpcf_collection_get(&DeviceData->CommonBuffers,
					ReqData->id);
		if (CommonBufferData == NULL) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		Status = PciDtfDmaReadWrite(ReqData,
					    CommonBufferData->VirtualAddress,
					    CommonBufferData->Length, Read);
	}
	__finally {
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

NTSTATUS PciDtfGetDma(IN PDEVICE_DATA DeviceData, IN PIRP Irp,
		      IN PIO_STACK_LOCATION IrpStack)
{
	PCIDTF_DMA_INFO *ReqData;
	PCOMMON_BUFFER_DATA CommonBufferData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (IrpStack->Parameters.DeviceIoControl.InputBufferLength !=
		    sizeof(PCIDTF_DMA_INFO)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength !=
		    sizeof(PCIDTF_DMA_INFO)) {
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_DMA_INFO *) Irp->AssociatedIrp.SystemBuffer;
		CommonBufferData = (PCOMMON_BUFFER_DATA)
		    xpcf_collection_get(&DeviceData->CommonBuffers,
					ReqData->id);
		if (CommonBufferData == NULL) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		ReqData->addr = CommonBufferData->PhysicalAddress.QuadPart;
		ReqData->len = CommonBufferData->Length;
		Irp->IoStatus.Information = sizeof(PCIDTF_DMA_INFO);
	}
	__finally {
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}
