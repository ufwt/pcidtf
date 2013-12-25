/*
 * PCI Device Test Framework
 * Windows kernel-mode driver (KMDF)
 * This file implements register space object functions.
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

EVT_WDF_OBJECT_CONTEXT_DESTROY EvtDestroyCallback;

NTSTATUS RegSpaceCreate(__in WDFDEVICE Device,
			__in PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDesc)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	WDF_OBJECT_ATTRIBUTES ObjectAttributes;
	WDFOBJECT RegSpace = NULL;
	PREG_SPACE_DATA RegSpaceData;
	NTSTATUS Status = STATUS_SUCCESS;

	__try {
		WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&ObjectAttributes,
							REG_SPACE_DATA);
		ObjectAttributes.ParentObject = Device;
		ObjectAttributes.EvtDestroyCallback = EvtDestroyCallback;

		Status = WdfObjectCreate(&ObjectAttributes, &RegSpace);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfObjectCreate", Status);
			__leave;
		}

		RegSpaceData = GetRegSpaceData(RegSpace);
		Status = PciDtfRegSpaceInit(RegSpaceData, ResourceDesc);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("PciDtfRegSpaceInit", Status);
			__leave;
		}

		Status = WdfCollectionAdd(DeviceData->RegSpaces, RegSpace);
		if (!NT_SUCCESS(Status)) {
			TRACE_ERR("WdfCollectionAdd", Status);
			__leave;
		}
	}
	__finally {
		if (!NT_SUCCESS(Status) && RegSpace != NULL)
			WdfObjectDelete(RegSpace);
	}
	return Status;
}

NTSTATUS PciDtfRegSpaceGet(IN WDFDEVICE Device, IN ULONG Index,
			   OUT WDFOBJECT * ppRegSpace)
{
	PDEVICE_DATA DeviceData = GetDeviceData(Device);
	WDFOBJECT RegSpace;
	NTSTATUS Status = STATUS_SUCCESS;

	RegSpace = WdfCollectionGetItem(DeviceData->RegSpaces, Index);
	if (RegSpace == NULL) {
		TRACE_MSG(TRACE_LEVEL_ERROR, TRACE_FLAG_REG_SPACE,
			  "Invalid index=%u\n", Index);
		Status = STATUS_INVALID_PARAMETER;
	} else {
		*ppRegSpace = RegSpace;
	}
	return Status;
}

VOID EvtDestroyCallback(IN WDFOBJECT Object)
{
	PciDtfRegSpaceCleanup(GetRegSpaceData(Object));
}
