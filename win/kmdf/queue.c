/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (KMDF)
 * This file implements I/O queue object functions.
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
#include "pcidtf_ioctl.h"

NTSTATUS PciDtfDeviceGetInfo(IN WDFDEVICE Device, IN WDFREQUEST Request);
NTSTATUS PciDtfDeviceReadWriteCfg(IN WDFDEVICE Device, IN WDFREQUEST Request,
				  IN BOOLEAN Read);
NTSTATUS PciDtfDeviceGetReg(IN WDFDEVICE Device, IN WDFREQUEST Request);
NTSTATUS PciDtfDeviceReadWriteReg(IN WDFDEVICE Device, IN WDFREQUEST Request,
				  IN BOOLEAN Read);
NTSTATUS PciDtfDeviceAllocDma(IN WDFDEVICE Device, IN WDFREQUEST Request);
NTSTATUS PciDtfDeviceFreeDma(IN WDFDEVICE Device, IN WDFREQUEST Request);
NTSTATUS PciDtfDeviceReadWriteDma(IN WDFDEVICE Device, IN WDFREQUEST Request,
				  IN BOOLEAN Read);
NTSTATUS PciDtfDeviceGetDma(IN WDFDEVICE Device, IN WDFREQUEST Request);

WDFCOMMONBUFFER PciDtfCommonBufferFind(IN PDEVICE_DATA DeviceData, IN int ID,
				       IN BOOLEAN Remove);
int PciDtfCommonBufferAssignId(IN PDEVICE_DATA DeviceData);

VOID
EvtIoDeviceControl(IN WDFQUEUE Queue,
		   IN WDFREQUEST Request,
		   IN size_t OutputBufferLength,
		   IN size_t InputBufferLength, IN ULONG IoControlCode)
{
	WDFDEVICE Device = WdfIoQueueGetDevice(Queue);

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	switch (IoControlCode) {
	case IOCTL_PCIDTF_GET_INFO:
		PciDtfDeviceGetInfo(Device, Request);
		break;
	case IOCTL_PCIDTF_READ_CFG:
		PciDtfDeviceReadWriteCfg(Device, Request, TRUE);
		break;
	case IOCTL_PCIDTF_WRITE_CFG:
		PciDtfDeviceReadWriteCfg(Device, Request, FALSE);
		break;
	case IOCTL_PCIDTF_GET_REG:
		PciDtfDeviceGetReg(Device, Request);
		break;
	case IOCTL_PCIDTF_READ_REG:
		PciDtfDeviceReadWriteReg(Device, Request, TRUE);
		break;
	case IOCTL_PCIDTF_WRITE_REG:
		PciDtfDeviceReadWriteReg(Device, Request, FALSE);
		break;
	case IOCTL_PCIDTF_ALLOC_DMA:
		PciDtfDeviceAllocDma(Device, Request);
		break;
	case IOCTL_PCIDTF_FREE_DMA:
		PciDtfDeviceFreeDma(Device, Request);
		break;
	case IOCTL_PCIDTF_READ_DMA:
		PciDtfDeviceReadWriteDma(Device, Request, TRUE);
		break;
	case IOCTL_PCIDTF_WRITE_DMA:
		PciDtfDeviceReadWriteDma(Device, Request, FALSE);
		break;
	case IOCTL_PCIDTF_GET_DMA_INFO:
		PciDtfDeviceGetDma(Device, Request);
		break;
	default:
		TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_QUEUE,
			  "Unsupported I/O control code=0x%X (0x%X)\n",
			  IoControlCode,
			  IoGetFunctionCodeFromCtlCode(IoControlCode));
		WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
		break;
	}
}

NTSTATUS PciDtfDeviceGetInfo(IN WDFDEVICE Device, IN WDFREQUEST Request)
{
	PCIDTF_DEV_INFO *ReqData;
	ULONG Value, Length;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		Status = WdfRequestRetrieveOutputBuffer(Request,
							sizeof(PCIDTF_DEV_INFO),
							(PVOID *) & ReqData,
							NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveOutputBuffer", Status);
			__leave;
		}
		Status = WdfDeviceQueryProperty(Device,
						DevicePropertyBusNumber,
						sizeof(Value), &Value, &Length);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR
			    ("WdfDeviceQueryProperty(DevicePropertyBusNumber)",
			     Status);
			__leave;
		}
		ReqData->bus = (UINT8) Value;
		Status = WdfDeviceQueryProperty(Device,
						DevicePropertyAddress,
						sizeof(Value), &Value, &Length);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR
			    ("WdfDeviceQueryProperty(DevicePropertyAddress)",
			     Status);
			__leave;
		}
		ReqData->devfn = (UINT8) Value;
		ReqData->reg_count = (int)
		    WdfCollectionGetCount(GetDeviceData(Device)->RegSpaces);
		TRACE_MSG(TRACE_LEVEL_VERBOSE, TRACE_FLAG_QUEUE,
			  "bus=0x%X, devfn=0x%X, reg_count=%d\n", ReqData->bus,
			  ReqData->devfn, ReqData->reg_count);
		WdfRequestSetInformation(Request, sizeof(PCIDTF_DEV_INFO));
	}
	__finally {
		WdfRequestComplete(Request, Status);
	}
	return Status;
}

NTSTATUS PciDtfDeviceReadWriteCfg(IN WDFDEVICE Device, IN WDFREQUEST Request,
				  IN BOOLEAN Read)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	PCIDTF_CFG_DATA *ReqData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		Status = WdfRequestRetrieveInputBuffer(Request,
						       sizeof(PCIDTF_CFG_DATA),
						       (PVOID *) & ReqData,
						       NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveInputBuffer", Status);
			__leave;
		}
		if (Read) {
			Status = WdfRequestRetrieveOutputBuffer(Request,
								sizeof
								(PCIDTF_CFG_DATA),
								(PVOID *) &
								ReqData, NULL);
			if (!NT_SUCCESS(Status)) {
				TRACE_ERR("WdfRequestRetrieveOutputBuffer",
					  Status);
				__leave;
			}
			Status =
			    PciDtfConfigRead(&DeviceData->BusIntf, ReqData->off,
					     ReqData->len, &ReqData->val);
			if (!NT_SUCCESS(Status)) {
				TRACE_ERR("PciDtfConfigRead", Status);
				__leave;
			}
			WdfRequestSetInformation(Request,
						 sizeof(PCIDTF_CFG_DATA));
		} else {
			Status = PciDtfConfigWrite(&DeviceData->BusIntf,
						   ReqData->off, ReqData->len,
						   ReqData->val);
			if (!NT_SUCCESS(Status)) {
				TRACE_ERR("PciDtfConfigWrite", Status);
				__leave;
			}
		}
	}
	__finally {
		WdfRequestComplete(Request, Status);
	}
	return Status;
}

NTSTATUS PciDtfDeviceGetReg(IN WDFDEVICE Device, IN WDFREQUEST Request)
{
	PCIDTF_REG_INFO *ReqData;
	WDFOBJECT RegSpace;
	PREG_SPACE_DATA RegSpaceData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		Status = WdfRequestRetrieveInputBuffer(Request,
						       sizeof(PCIDTF_REG_INFO),
						       (PVOID *) & ReqData,
						       NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveInputBuffer", Status);
			__leave;
		}
		Status = PciDtfRegSpaceGet(Device, ReqData->bar, &RegSpace);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("PciDtfRegSpaceGet", Status);
			__leave;
		}
		RegSpaceData = GetRegSpaceData(RegSpace);
		if (RegSpaceData->ResourceDesc.Type == CmResourceTypePort) {
			ReqData->len = RegSpaceData->ResourceDesc.u.Port.Length;
			ReqData->addr =
			    RegSpaceData->ResourceDesc.u.Port.Start.QuadPart;
		} else {
			ReqData->len =
			    RegSpaceData->ResourceDesc.u.Memory.Length;
			ReqData->addr =
			    RegSpaceData->ResourceDesc.u.Memory.Start.QuadPart;
		}
		WdfRequestSetInformation(Request, sizeof(PCIDTF_REG_INFO));
	}
	__finally {
		WdfRequestComplete(Request, Status);
	}
	return Status;
}

NTSTATUS PciDtfDeviceReadWriteReg(IN WDFDEVICE Device, IN WDFREQUEST Request,
				  IN BOOLEAN Read)
{
	PCIDTF_REG_DATA *ReqData;
	PREG_SPACE_DATA RegSpaceData;
	WDFOBJECT RegSpace;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		Status = WdfRequestRetrieveInputBuffer(Request,
						       sizeof(PCIDTF_REG_DATA),
						       (PVOID *) & ReqData,
						       NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveInputBuffer", Status);
			__leave;
		}
		Status = PciDtfRegSpaceGet(Device, ReqData->bar, &RegSpace);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("PciDtfRegSpaceGet", Status);
			__leave;
		}
		RegSpaceData = GetRegSpaceData(RegSpace);
		if (Read) {
			Status = WdfRequestRetrieveOutputBuffer(Request,
								sizeof
								(PCIDTF_REG_DATA),
								(PVOID *) &
								ReqData, NULL);
			if (!NT_SUCCESS(Status)) {
				TRACE_ERR("WdfRequestRetrieveOutputBuffer",
					  Status);
				__leave;
			}
			Status = PciDtfRegSpaceRead(RegSpaceData,
						    ReqData->off, ReqData->len,
						    &ReqData->val);
			if (!NT_SUCCESS(Status)) {
				TRACE_ERR("PciDtfRegSpaceRead", Status);
				__leave;
			}
			WdfRequestSetInformation(Request,
						 sizeof(PCIDTF_REG_DATA));
		} else {
			Status = PciDtfRegSpaceWrite(RegSpaceData,
						     ReqData->off, ReqData->len,
						     ReqData->val);
			if (!NT_SUCCESS(Status)) {
				TRACE_ERR("PciDtfRegSpaceWrite", Status);
				__leave;
			}
		}
	}
	__finally {
		WdfRequestComplete(Request, Status);
	}
	return Status;
}

NTSTATUS PciDtfDeviceAllocDma(IN WDFDEVICE Device, IN WDFREQUEST Request)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	PCIDTF_DMA_INFO *ReqData;
	WDF_OBJECT_ATTRIBUTES ObjectAttributes;
	WDFCOMMONBUFFER CommonBuffer = NULL;
	PCOMMON_BUFFER_DATA CommonBufferData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		Status = WdfRequestRetrieveInputBuffer(Request,
						       sizeof(PCIDTF_DMA_INFO),
						       (PVOID *) & ReqData,
						       NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveInputBuffer", Status);
			__leave;
		}
		Status = WdfRequestRetrieveOutputBuffer(Request,
							sizeof(PCIDTF_DMA_INFO),
							(PVOID *) & ReqData,
							NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveOutputBuffer", Status);
			__leave;
		}
		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ObjectAttributes,
							COMMON_BUFFER_DATA);
		Status = WdfCommonBufferCreate(DeviceData->DmaEnabler,
					       ReqData->len, &ObjectAttributes,
					       &CommonBuffer);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfCommonBufferCreate", Status);
			__leave;
		}
		CommonBufferData = GetCommonBufferData(CommonBuffer);
		CommonBufferData->ID = PciDtfCommonBufferAssignId(DeviceData);
		Status = WdfCollectionAdd(DeviceData->CommonBuffers,
					  CommonBuffer);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfCollectionAdd", Status);
			__leave;
		}
		ReqData->id = CommonBufferData->ID;
		ReqData->addr =
		    WdfCommonBufferGetAlignedLogicalAddress
		    (CommonBuffer).QuadPart;
		WdfRequestSetInformation(Request, sizeof(PCIDTF_DMA_INFO));

		TRACE_MSG(TRACE_LEVEL_VERBOSE, TRACE_FLAG_QUEUE,
			  "va 0x%p, pa 0x%llX, len 0x%X\n",
			  WdfCommonBufferGetAlignedVirtualAddress(CommonBuffer),
			  WdfCommonBufferGetAlignedLogicalAddress
			  (CommonBuffer).QuadPart,
			  WdfCommonBufferGetLength(CommonBuffer));
	}
	__finally {
		if (!NT_SUCCESS(Status) && CommonBuffer != NULL) {
			WdfObjectDelete(CommonBuffer);
		}
		WdfRequestComplete(Request, Status);
	}
	return Status;
}

NTSTATUS PciDtfDeviceFreeDma(IN WDFDEVICE Device, IN WDFREQUEST Request)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	int *ID;
	WDFCOMMONBUFFER CommonBuffer;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		Status = WdfRequestRetrieveInputBuffer(Request,
						       sizeof(int),
						       (PVOID *) & ID, NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveInputBuffer", Status);
			__leave;
		}
		CommonBuffer = PciDtfCommonBufferFind(DeviceData, *ID, TRUE);
		if (CommonBuffer == NULL) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		TRACE_MSG(TRACE_LEVEL_VERBOSE, TRACE_FLAG_QUEUE,
			  "Delete common buffer 0x%p (ID=%d)\n", CommonBuffer,
			  *ID);
		WdfObjectDelete(CommonBuffer);
	}
	__finally {
		WdfRequestComplete(Request, Status);
	}
	return Status;
}

NTSTATUS PciDtfDeviceReadWriteDma(IN WDFDEVICE Device, IN WDFREQUEST Request,
				  IN BOOLEAN Read)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	PCIDTF_DMA_DATA *ReqData;
	WDFCOMMONBUFFER CommonBuffer;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		Status = WdfRequestRetrieveInputBuffer(Request,
						       sizeof(PCIDTF_DMA_DATA),
						       (PVOID *) & ReqData,
						       NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveInputBuffer", Status);
			__leave;
		}
		TRACE_MSG(TRACE_LEVEL_VERBOSE, TRACE_FLAG_QUEUE,
			  "id=%d, off=%d, len=%d, buf=0x%p, read=%u\n",
			  ReqData->id, ReqData->off, ReqData->len, ReqData->buf,
			  Read);
		CommonBuffer = PciDtfCommonBufferFind(DeviceData,
						      ReqData->id, FALSE);
		if (CommonBuffer == NULL) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		Status = PciDtfDmaReadWrite(ReqData,
					    WdfCommonBufferGetAlignedVirtualAddress
					    (CommonBuffer),
					    WdfCommonBufferGetLength
					    (CommonBuffer), Read);
	}
	__finally {
		WdfRequestComplete(Request, Status);
	}
	return Status;
}

NTSTATUS PciDtfDeviceGetDma(IN WDFDEVICE Device, IN WDFREQUEST Request)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	PCIDTF_DMA_INFO *ReqData;
	WDFCOMMONBUFFER CommonBuffer;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		Status = WdfRequestRetrieveInputBuffer(Request,
						       sizeof(PCIDTF_DMA_INFO),
						       (PVOID *) & ReqData,
						       NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveInputBuffer", Status);
			__leave;
		}
		Status = WdfRequestRetrieveOutputBuffer(Request,
							sizeof(PCIDTF_DMA_INFO),
							(PVOID *) & ReqData,
							NULL);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfRequestRetrieveOutputBuffer", Status);
			__leave;
		}
		CommonBuffer =
		    PciDtfCommonBufferFind(DeviceData, ReqData->id, FALSE);
		if (CommonBuffer == NULL) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		ReqData->addr =
		    WdfCommonBufferGetAlignedLogicalAddress
		    (CommonBuffer).QuadPart;
		ReqData->len = (int)WdfCommonBufferGetLength(CommonBuffer);
		WdfRequestSetInformation(Request, sizeof(PCIDTF_DMA_INFO));
	}
	__finally {
		WdfRequestComplete(Request, Status);
	}
	return Status;
}

WDFCOMMONBUFFER PciDtfCommonBufferFind(IN PDEVICE_DATA DeviceData, IN int ID,
				       IN BOOLEAN Remove)
{
	WDFCOMMONBUFFER CommonBuffer = NULL;
	ULONG Index, Count;

	Count = WdfCollectionGetCount(DeviceData->CommonBuffers);
	for (Index = 0; Index < Count; Index++) {
		CommonBuffer = (WDFCOMMONBUFFER)
		    WdfCollectionGetItem(DeviceData->CommonBuffers, Index);
		if (GetCommonBufferData(CommonBuffer)->ID == ID) {
			if (Remove)
				WdfCollectionRemoveItem
				    (DeviceData->CommonBuffers, Index);
			break;
		}
	}
	return Index < Count ? CommonBuffer : NULL;
}

int PciDtfCommonBufferAssignId(IN PDEVICE_DATA DeviceData)
{
	int ID;

	for (ID = 1;; ID++) {
		if (PciDtfCommonBufferFind(DeviceData, ID, FALSE) == NULL)
			break;
	}
	return ID;
}
