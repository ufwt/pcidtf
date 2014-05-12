/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (WDM)
 * This file implements DMA management functions.
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

NTSTATUS PciDtfDmaCreate(IN PDEVICE_DATA DeviceData, IN ULONG Length,
			 OUT PCOMMON_BUFFER_DATA * ppBufData, OUT int *id)
{
	PCOMMON_BUFFER_DATA BufData = NULL;
	PDMA_OPERATIONS DmaOps = DeviceData->DmaOperations;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		BufData =
		    (PCOMMON_BUFFER_DATA) ExAllocatePoolWithTag(NonPagedPool,
								sizeof
								(COMMON_BUFFER_DATA),
								PCIDTF_POOL_TAG);
		if (BufData == NULL) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		RtlZeroMemory(BufData, sizeof(PCOMMON_BUFFER_DATA));
		BufData->VirtualAddress =
		    DmaOps->AllocateCommonBuffer(DeviceData->DmaAdapter,
						 Length,
						 &BufData->PhysicalAddress,
						 FALSE);
		if (BufData->VirtualAddress == NULL) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		if (xpcf_collection_add
		    (&DeviceData->CommonBuffers, BufData, id)) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DMA,
			  "Common buffer allocated (VA 0x%p, PA 0x%llX)\n",
			  BufData->VirtualAddress,
			  BufData->PhysicalAddress.QuadPart);
		BufData->Length = Length;
		*ppBufData = BufData;
	}
	__finally {
		if (!NT_SUCCESS(Status) && BufData != NULL) {
			PciDtfCleanupCommonBuffer(&DeviceData->CommonBuffers,
						  BufData);
		}
	}
	return Status;
}

XPCF_CB(void)PciDtfCleanupCommonBuffer(XPCF_COLLECTION * col, void *item)
{
	PDEVICE_DATA DeviceData = (PDEVICE_DATA) col->ctx;
	PCOMMON_BUFFER_DATA BufData = (PCOMMON_BUFFER_DATA) item;
	PDMA_OPERATIONS DmaOps = DeviceData->DmaOperations;

	if (BufData->VirtualAddress) {
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DMA,
			  "Free common buffer (VA 0x%p, PA 0x%llX)\n",
			  BufData->VirtualAddress,
			  BufData->PhysicalAddress.QuadPart);
		DmaOps->FreeCommonBuffer(DeviceData->DmaAdapter,
					 BufData->Length,
					 BufData->PhysicalAddress,
					 BufData->VirtualAddress, FALSE);
	}
	ExFreePoolWithTag(BufData, PCIDTF_POOL_TAG);
}
