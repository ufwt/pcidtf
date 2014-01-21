/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (WDM)
 * This file implements I/O control functions.
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
#include "pcidtf_ioctl.h"

//
// Local function prototypes
//

NTSTATUS PciDtfGetInfo(IN BASE_FILE * File, IN PIRP Irp,
		       IN PIO_STACK_LOCATION IrpStack);
NTSTATUS PciDtfReadWriteCfg(IN BASE_FILE * File, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read);
NTSTATUS PciDtfGetReg(IN BASE_FILE * File, IN PIRP Irp,
		      IN PIO_STACK_LOCATION IrpStack);
NTSTATUS PciDtfAllocDma(IN BASE_FILE * File, IN PIRP Irp,
			IN PIO_STACK_LOCATION IrpStack);
NTSTATUS PciDtfFreeDma(IN BASE_FILE * File, IN PIRP Irp,
		       IN PIO_STACK_LOCATION IrpStack);
NTSTATUS PciDtfReadWriteReg(IN BASE_FILE * File, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read);
NTSTATUS PciDtfReadWriteDma(IN BASE_FILE * File, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read);
NTSTATUS PciDtfGetDma(IN BASE_FILE * File, IN PIRP Irp,
		      IN PIO_STACK_LOCATION IrpStack);

NTSTATUS PciDtfFileIoctl(__in BASE_FILE * File, __in PIRP Irp,
			 __in PIO_STACK_LOCATION IrpStack)
{
	NTSTATUS Status;

	switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {

	case IOCTL_PCIDTF_GET_INFO:
		Status = PciDtfGetInfo(File, Irp, IrpStack);
		break;

	case IOCTL_PCIDTF_READ_CFG:
		Status = PciDtfReadWriteCfg(File, Irp, IrpStack, TRUE);
		break;

	case IOCTL_PCIDTF_WRITE_CFG:
		Status = PciDtfReadWriteCfg(File, Irp, IrpStack, FALSE);
		break;

	case IOCTL_PCIDTF_GET_REG:
		Status = PciDtfGetReg(File, Irp, IrpStack);
		break;

	case IOCTL_PCIDTF_READ_REG:
		Status = PciDtfReadWriteReg(File, Irp, IrpStack, TRUE);
		break;

	case IOCTL_PCIDTF_WRITE_REG:
		Status = PciDtfReadWriteReg(File, Irp, IrpStack, FALSE);
		break;

	case IOCTL_PCIDTF_ALLOC_DMA:
		Status = PciDtfAllocDma(File, Irp, IrpStack);
		break;

	case IOCTL_PCIDTF_FREE_DMA:
		Status = PciDtfFreeDma(File, Irp, IrpStack);
		break;

	case IOCTL_PCIDTF_READ_DMA:
		Status = PciDtfReadWriteDma(File, Irp, IrpStack, TRUE);
		break;

	case IOCTL_PCIDTF_WRITE_DMA:
		Status = PciDtfReadWriteDma(File, Irp, IrpStack, FALSE);
		break;

	case IOCTL_PCIDTF_GET_DMA_INFO:
		Status = PciDtfGetDma(File, Irp, IrpStack);
		break;

	default:
		TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
			  "Unsupported I/O control code 0x%X\n",
			  IrpStack->Parameters.DeviceIoControl.IoControlCode);
		Status =
		    BaseFileCompleteRequest(File, Irp,
					    STATUS_INVALID_DEVICE_REQUEST);
		break;
	}
	return Status;
}

// Implement local functions

NTSTATUS PciDtfGetInfo(IN BASE_FILE * File, IN PIRP Irp,
		       IN PIO_STACK_LOCATION IrpStack)
{
	BASE_DEVICE *Device = BaseFileGetDevice(File);
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	PDEVICE_OBJECT PhysicalDeviceObject =
	    BaseDeviceGetPhysicalDeviceObject(Device);
	PCIDTF_DEV_INFO *ReqData;
	ULONG Value, Length;
	ULONG OutputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (OutputBufferLength != sizeof(PCIDTF_DEV_INFO)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid output buffer length=%u\n",
				  OutputBufferLength);
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_DEV_INFO *) Irp->AssociatedIrp.SystemBuffer;

		Status = IoGetDeviceProperty(PhysicalDeviceObject,
					     DevicePropertyBusNumber,
					     sizeof(Value), &Value, &Length);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR
			    ("IoGetDeviceProperty(DevicePropertyBusNumber)",
			     Status);
			__leave;
		}
		ReqData->bus = (UINT8) Value;

		Status = IoGetDeviceProperty(PhysicalDeviceObject,
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
		BaseFileCompleteRequest(File, Irp, Status);
	}
	return Status;
}

NTSTATUS PciDtfReadWriteCfg(IN BASE_FILE * File, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read)
{
	BASE_DEVICE *Device = BaseFileGetDevice(File);
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	PCIDTF_CFG_DATA *ReqData;
	ULONG InputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (InputBufferLength != sizeof(PCIDTF_CFG_DATA)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid input buffer length=%u\n",
				  InputBufferLength);
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_CFG_DATA *) Irp->AssociatedIrp.SystemBuffer;
		if (Read) {
			if (OutputBufferLength != sizeof(PCIDTF_CFG_DATA)) {
				TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
					  "Invalid output buffer length=%u\n",
					  OutputBufferLength);
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
		BaseFileCompleteRequest(File, Irp, Status);
	}
	return Status;
}

NTSTATUS PciDtfGetReg(IN BASE_FILE * File, IN PIRP Irp,
		      IN PIO_STACK_LOCATION IrpStack)
{
	BASE_DEVICE *Device = BaseFileGetDevice(File);
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	PCIDTF_REG_INFO *ReqData;
	PREG_SPACE_DATA RegSpaceData;
	ULONG InputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (InputBufferLength != sizeof(PCIDTF_REG_INFO)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid input buffer length=%u\n",
				  InputBufferLength);
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		if (OutputBufferLength != sizeof(PCIDTF_REG_INFO)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid output buffer length=%u\n",
				  OutputBufferLength);
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
		BaseFileCompleteRequest(File, Irp, Status);
	}
	return Status;
}

NTSTATUS PciDtfReadWriteReg(IN BASE_FILE * File, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read)
{
	BASE_DEVICE *Device = BaseFileGetDevice(File);
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	PCIDTF_REG_DATA *ReqData;
	PREG_SPACE_DATA RegSpaceData;
	ULONG InputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (InputBufferLength != sizeof(PCIDTF_REG_DATA)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid input buffer length=%u\n",
				  InputBufferLength);
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
			if (OutputBufferLength != sizeof(PCIDTF_REG_DATA)) {
				TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
					  "Invalid output buffer length=%u\n",
					  OutputBufferLength);
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
		BaseFileCompleteRequest(File, Irp, Status);
	}
	return Status;
}

NTSTATUS PciDtfAllocDma(IN BASE_FILE * File, IN PIRP Irp,
			IN PIO_STACK_LOCATION IrpStack)
{
	BASE_DEVICE *Device = BaseFileGetDevice(File);
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	PCIDTF_DMA_INFO *ReqData;
	PCOMMON_BUFFER_DATA BufData;
	ULONG InputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (InputBufferLength != sizeof(PCIDTF_DMA_INFO)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid input buffer length=%u\n",
				  InputBufferLength);
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		if (OutputBufferLength != sizeof(PCIDTF_DMA_INFO)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid output buffer length=%u\n",
				  OutputBufferLength);
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_DMA_INFO *) Irp->AssociatedIrp.SystemBuffer;
		Status = PciDtfDmaCreate(DeviceData, ReqData->len,
					 &BufData, &ReqData->id);
		if (!NT_SUCCESS(Status)) {
			__leave;
		}
		ReqData->addr = BufData->PhysicalAddress.QuadPart;
		Irp->IoStatus.Information = sizeof(PCIDTF_DMA_INFO);
	}
	__finally {
		BaseFileCompleteRequest(File, Irp, Status);
	}
	return Status;
}

NTSTATUS PciDtfFreeDma(IN BASE_FILE * File, IN PIRP Irp,
		       IN PIO_STACK_LOCATION IrpStack)
{
	BASE_DEVICE *Device = BaseFileGetDevice(File);
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	int id;
	ULONG InputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (InputBufferLength != sizeof(int)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid input buffer length=%u\n",
				  InputBufferLength);
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
		BaseFileCompleteRequest(File, Irp, Status);
	}
	return Status;
}

NTSTATUS PciDtfReadWriteDma(IN BASE_FILE * File, IN PIRP Irp,
			    IN PIO_STACK_LOCATION IrpStack, IN BOOLEAN Read)
{
	BASE_DEVICE *Device = BaseFileGetDevice(File);
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	PCIDTF_DMA_DATA *ReqData;
	PCOMMON_BUFFER_DATA BufData;
	ULONG InputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (InputBufferLength != sizeof(PCIDTF_DMA_DATA)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid input buffer length=%u\n",
				  InputBufferLength);
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		if (Read && OutputBufferLength != sizeof(PCIDTF_DMA_DATA)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid output buffer length=%u\n",
				  OutputBufferLength);
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_DMA_DATA *) Irp->AssociatedIrp.SystemBuffer;
		BufData = (PCOMMON_BUFFER_DATA)
		    xpcf_collection_get(&DeviceData->CommonBuffers,
					ReqData->id);
		if (BufData == NULL) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		Status = PciDtfDmaReadWrite(ReqData,
					    BufData->VirtualAddress,
					    BufData->Length, Read);
	}
	__finally {
		BaseFileCompleteRequest(File, Irp, Status);
	}
	return Status;
}

NTSTATUS PciDtfGetDma(IN BASE_FILE * File, IN PIRP Irp,
		      IN PIO_STACK_LOCATION IrpStack)
{
	BASE_DEVICE *Device = BaseFileGetDevice(File);
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) BaseDeviceGetPrivate(Device);
	PCIDTF_DMA_INFO *ReqData;
	PCOMMON_BUFFER_DATA BufData;
	ULONG InputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferLength =
	    IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (InputBufferLength != sizeof(PCIDTF_DMA_INFO)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid input buffer length=%u\n",
				  InputBufferLength);
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		if (OutputBufferLength != sizeof(PCIDTF_DMA_INFO)) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_IOCTL,
				  "Invalid output buffer length=%u\n",
				  OutputBufferLength);
			Status = STATUS_INVALID_BUFFER_SIZE;
			__leave;
		}
		ReqData = (PCIDTF_DMA_INFO *) Irp->AssociatedIrp.SystemBuffer;
		BufData = (PCOMMON_BUFFER_DATA)
		    xpcf_collection_get(&DeviceData->CommonBuffers,
					ReqData->id);
		if (BufData == NULL) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		ReqData->addr = BufData->PhysicalAddress.QuadPart;
		ReqData->len = BufData->Length;
		Irp->IoStatus.Information = sizeof(PCIDTF_DMA_INFO);
	}
	__finally {
		BaseFileCompleteRequest(File, Irp, Status);
	}
	return Status;
}
