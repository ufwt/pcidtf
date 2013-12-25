/*
 * PCI Device Test Framework
 * This file implements kernel-mode common functions shared by WDM and KMDF
 * drivers. 
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
#include "pcidtf_common.h"

NTSTATUS PciDtfConfigRead(IN PBUS_INTERFACE_STANDARD BusIntf, IN ULONG Offset,
			  IN ULONG Length, OUT UINT32 * Value)
{
	ULONG Ret;
	NTSTATUS Status = STATUS_SUCCESS;

	if (Length <= sizeof(UINT32)) {
		ASSERT(BusIntf->GetBusData != NULL);
		*Value = 0;
		Ret =
		    BusIntf->GetBusData(BusIntf->Context, PCI_WHICHSPACE_CONFIG,
					Value, Offset, Length);
		if (Ret != Length) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
				  "GetBusData failed (ret=%u)\n", Ret);
			Status = STATUS_UNSUCCESSFUL;
		}
	} else {
		TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
			  "Invalid length=%u\n", Length);
		Status = STATUS_INVALID_PARAMETER;
	}
	return Status;
}

NTSTATUS PciDtfConfigWrite(IN PBUS_INTERFACE_STANDARD BusIntf, IN ULONG Offset,
			   IN ULONG Length, IN UINT32 Value)
{
	ULONG Ret;
	NTSTATUS Status = STATUS_SUCCESS;

	if (Length <= sizeof(UINT32)) {
		ASSERT(BusIntf->SetBusData != NULL);
		Ret =
		    BusIntf->SetBusData(BusIntf->Context, PCI_WHICHSPACE_CONFIG,
					&Value, Offset, Length);
		if (Ret != Length) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
				  "SetBusData failed (ret=%u)\n", Ret);
			Status = STATUS_UNSUCCESSFUL;
		}
	} else {
		TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
			  "Invalid length=%u\n", Length);
		Status = STATUS_INVALID_PARAMETER;
	}
	return Status;
}

NTSTATUS PciDtfRegSpaceInit(IN PREG_SPACE_DATA RegSpaceData,
			    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDesc)
{
	MEMORY_CACHING_TYPE CacheType;
	NTSTATUS Status = STATUS_SUCCESS;

	RegSpaceData->ResourceDesc = *ResourceDesc;

	if (ResourceDesc->Type == CmResourceTypeMemory) {
		if (ResourceDesc->Flags & CM_RESOURCE_MEMORY_CACHEABLE)
			CacheType = MmCached;
		else if (ResourceDesc->Flags & CM_RESOURCE_MEMORY_COMBINEDWRITE)
			CacheType = MmWriteCombined;
		else
			CacheType = MmNonCached;
		RegSpaceData->MappedIoSpace =
		    MmMapIoSpace(ResourceDesc->u.Memory.Start,
				 ResourceDesc->u.Memory.Length, CacheType);
		if (RegSpaceData->MappedIoSpace == NULL) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
				  "MmMapIoSpace failed\n");
			Status = STATUS_INSUFFICIENT_RESOURCES;
		} else {
			TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DEFAULT,
				  "MappedIoSpace=0x%p\n",
				  RegSpaceData->MappedIoSpace);
		}
	} else {
		RegSpaceData->MappedIoSpace = NULL;
	}
	return Status;
}

VOID PciDtfRegSpaceCleanup(IN PREG_SPACE_DATA RegSpaceData)
{
	PVOID MappedIoSpace;

	MappedIoSpace =
	    InterlockedExchangePointer(&RegSpaceData->MappedIoSpace, NULL);
	if (MappedIoSpace != NULL) {
		ASSERT(RegSpaceData->ResourceDesc.Type == CmResourceTypeMemory);
		TRACE_MSG(TRACE_LEVEL_INFO, TRACE_FLAG_DEFAULT,
			  "Unmap I/O space 0x%p\n", MappedIoSpace);
		MmUnmapIoSpace(MappedIoSpace,
			       RegSpaceData->ResourceDesc.u.Memory.Length);
	}
}

NTSTATUS PciDtfRegSpaceRead(IN PREG_SPACE_DATA RegSpaceData, IN ULONG Offset,
			    IN ULONG Length, OUT PULONGLONG Value)
{
	LONGLONG StartAddr;
	PVOID RegAddr;
	NTSTATUS Status = STATUS_SUCCESS;

	if (RegSpaceData->ResourceDesc.Type == CmResourceTypePort) {
		if (Offset + Length > RegSpaceData->ResourceDesc.u.Port.Length) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
				  "Invalid parameter (offset=%u, length=%u)\n",
				  Offset, Length);
			Status = STATUS_INVALID_PARAMETER;
		} else {
			StartAddr =
			    RegSpaceData->ResourceDesc.u.Port.Start.QuadPart;
			RegAddr = (PUCHAR) (ULONG_PTR) StartAddr + Offset;
			switch (Length) {
			case sizeof(UCHAR):
				*Value = READ_PORT_UCHAR((PUCHAR) RegAddr);
				break;
			case sizeof(USHORT):
				*Value = READ_PORT_USHORT((PUSHORT) RegAddr);
				break;
			case sizeof(ULONG):
				*Value = READ_PORT_ULONG((PULONG) RegAddr);
				break;
			default:
				TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
					  "Invalid length=%u\n", Length);
				Status = STATUS_INVALID_PARAMETER;
				break;
			}
		}
	} else {
		if (Offset + Length >
		    RegSpaceData->ResourceDesc.u.Memory.Length) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
				  "Invalid parameter (offset=%u, length=%u)\n",
				  Offset, Length);
			Status = STATUS_INVALID_PARAMETER;
		} else {
			RegAddr = (PUCHAR) RegSpaceData->MappedIoSpace + Offset;
			switch (Length) {
			case sizeof(UCHAR):
				*Value = READ_REGISTER_UCHAR((PUCHAR) RegAddr);
				break;
			case sizeof(USHORT):
				*Value =
				    READ_REGISTER_USHORT((PUSHORT) RegAddr);
				break;
			case sizeof(ULONG):
				*Value = READ_REGISTER_ULONG((PULONG) RegAddr);
				break;
#ifdef WIN64
			case sizeof(ULONGLONG):
				*Value =
				    READ_REGISTER_ULONG64((PULONGLONG) RegAddr);
				break;
#endif
			default:
				TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
					  "Invalid length=%u\n", Length);
				Status = STATUS_INVALID_PARAMETER;
				break;
			}
		}
	}
	return Status;
}

NTSTATUS PciDtfRegSpaceWrite(IN PREG_SPACE_DATA RegSpaceData, IN ULONG Offset,
			     IN ULONG Length, IN ULONGLONG Value)
{
	LONGLONG StartAddr;
	PVOID RegAddr;
	NTSTATUS Status = STATUS_SUCCESS;

	if (RegSpaceData->ResourceDesc.Type == CmResourceTypePort) {
		if (Offset + Length > RegSpaceData->ResourceDesc.u.Port.Length) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
				  "Invalid parameter (offset=%u, length=%u)\n",
				  Offset, Length);
			Status = STATUS_INVALID_PARAMETER;
		} else {
			StartAddr =
			    RegSpaceData->ResourceDesc.u.Port.Start.QuadPart;
			RegAddr = (PUCHAR) (ULONG_PTR) StartAddr + Offset;
			switch (Length) {
			case sizeof(UCHAR):
				WRITE_PORT_UCHAR((PUCHAR) RegAddr,
						 (UCHAR) Value);
				break;
			case sizeof(USHORT):
				WRITE_PORT_USHORT((PUSHORT) RegAddr,
						  (USHORT) Value);
				break;
			case sizeof(ULONG):
				WRITE_PORT_ULONG((PULONG) RegAddr,
						 (ULONG) Value);
				break;
			default:
				TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
					  "Invalid length=%u\n", Length);
				Status = STATUS_INVALID_PARAMETER;
				break;
			}
		}
	} else {
		if (Offset + Length >
		    RegSpaceData->ResourceDesc.u.Memory.Length) {
			TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
				  "Invalid parameter (offset=%u, length=%u)\n",
				  Offset, Length);
			Status = STATUS_INVALID_PARAMETER;
		} else {
			RegAddr = (PUCHAR) RegSpaceData->MappedIoSpace + Offset;
			switch (Length) {
			case sizeof(UCHAR):
				WRITE_REGISTER_UCHAR((PUCHAR) RegAddr,
						     (UCHAR) Value);
				break;
			case sizeof(USHORT):
				WRITE_REGISTER_USHORT((PUSHORT) RegAddr,
						      (USHORT) Value);
				break;
			case sizeof(ULONG):
				WRITE_REGISTER_ULONG((PULONG) RegAddr,
						     (ULONG) Value);
				break;
#ifdef WIN64
			case sizeof(ULONGLONG):
				WRITE_REGISTER_ULONG64((PULONGLONG) RegAddr,
						       Value);
				break;
#endif
			default:
				TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_DEFAULT,
					  "Invalid length=%u\n", Length);
				Status = STATUS_INVALID_PARAMETER;
				break;
			}
		}
	}
	return Status;
}

NTSTATUS PciDtfDmaReadWrite(IN PCIDTF_DMA_DATA * ReqData,
			    IN PVOID VirtualAddress, IN ULONG Length,
			    IN BOOLEAN Read)
{
	PMDL Mdl = NULL;
	PVOID UserBuffer, Pointer;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		if (ReqData->off + ReqData->len > (int)Length) {
			Status = STATUS_INVALID_PARAMETER;
			__leave;
		}
		Mdl = IoAllocateMdl(ReqData->buf, ReqData->len, FALSE,
				    FALSE, NULL);
		if (Mdl == NULL) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			TRACE_ERR("IoAllocateMdl", Status);
			__leave;
		}
		Pointer = (PUCHAR) VirtualAddress + ReqData->off;
		if (Read) {
			__try {
				MmProbeAndLockPages(Mdl, KernelMode,
						    IoWriteAccess);
				UserBuffer = MmGetSystemAddressForMdlSafe(Mdl,
									  NormalPagePriority);
				if (UserBuffer == NULL) {
					Status = STATUS_INSUFFICIENT_RESOURCES;
					TRACE_ERR
					    ("MmGetSystemAddressForMdlSafe",
					     Status);
				} else {
					RtlCopyMemory(UserBuffer, Pointer,
						      ReqData->len);
				}
				MmUnlockPages(Mdl);
			}
			__except(EXCEPTION_EXECUTE_HANDLER) {
				Status = STATUS_INSUFFICIENT_RESOURCES;
			}
		} else {
			__try {
				MmProbeAndLockPages(Mdl, KernelMode,
						    IoReadAccess);
				UserBuffer = MmGetSystemAddressForMdlSafe(Mdl,
									  NormalPagePriority);
				if (UserBuffer == NULL) {
					Status = STATUS_INSUFFICIENT_RESOURCES;
					TRACE_ERR
					    ("MmGetSystemAddressForMdlSafe",
					     Status);
				} else {
					RtlCopyMemory(Pointer, UserBuffer,
						      ReqData->len);
				}
				MmUnlockPages(Mdl);
			}
			__except(EXCEPTION_EXECUTE_HANDLER) {
				Status = STATUS_INSUFFICIENT_RESOURCES;
			}
		}
	}
	__finally {
		if (Mdl)
			IoFreeMdl(Mdl);
	}
	return Status;
}
