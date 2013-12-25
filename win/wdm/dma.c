/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (WDM)
 * This file implements DMA management functions.
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

NTSTATUS PciDtfDmaCreate(IN PDEVICE_DATA DeviceData, IN ULONG Length,
			 OUT PCOMMON_BUFFER_DATA * ppCommonBufferData,
			 OUT int *id)
{
	PCOMMON_BUFFER_DATA CommonBufferData = NULL;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		CommonBufferData =
		    (PCOMMON_BUFFER_DATA) ExAllocatePoolWithTag(NonPagedPool,
								sizeof
								(COMMON_BUFFER_DATA),
								PCIDTF_POOL_TAG);
		if (CommonBufferData == NULL) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		RtlZeroMemory(CommonBufferData, sizeof(PCOMMON_BUFFER_DATA));
		CommonBufferData->VirtualAddress =
		    DeviceData->DmaAdapter->
		    DmaOperations->AllocateCommonBuffer(DeviceData->DmaAdapter,
							Length,
							&CommonBufferData->
							PhysicalAddress, FALSE);
		if (CommonBufferData->VirtualAddress == NULL) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		if (xpcf_collection_add
		    (&DeviceData->CommonBuffers, CommonBufferData, id)) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DMA,
			  "Common buffer allocated (VA 0x%p, PA 0x%llX)\n",
			  CommonBufferData->VirtualAddress,
			  CommonBufferData->PhysicalAddress.QuadPart);
		CommonBufferData->Length = Length;
		*ppCommonBufferData = CommonBufferData;
	}
	__finally {
		if (!NT_SUCCESS(Status) && CommonBufferData != NULL) {
			PciDtfCleanupCommonBuffer(&DeviceData->CommonBuffers,
						  CommonBufferData);
		}
	}
	return Status;
}

void PciDtfCleanupCommonBuffer(XPCF_COLLECTION * col, void *item)
{
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) col->ctx;
	PCOMMON_BUFFER_DATA CommonBufferData = (PCOMMON_BUFFER_DATA) item;

	if (CommonBufferData->VirtualAddress) {
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DMA,
			  "Free common buffer (VA 0x%p, PA 0x%llX)\n",
			  CommonBufferData->VirtualAddress,
			  CommonBufferData->PhysicalAddress.QuadPart);
		DeviceData->DmaAdapter->
		    DmaOperations->FreeCommonBuffer(DeviceData->DmaAdapter,
						    CommonBufferData->Length,
						    CommonBufferData->
						    PhysicalAddress,
						    CommonBufferData->
						    VirtualAddress, FALSE);
	}
	ExFreePoolWithTag(CommonBufferData, PCIDTF_POOL_TAG);
}
