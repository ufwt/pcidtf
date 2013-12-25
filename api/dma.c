/*
 * PCI Device Test Framework
 * User-mode Framework Library
 * This file implements DMA buffer management functions.
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

#include "pcidtf_def.h"

/* Local function prototype */
static PCIDTF_DMA *pcidtf_dev_add_dma(PCIDTF_DEV * dev, int id, int len,
				      unsigned long long addr);

PCIDTF_API_IMP(PCIDTF_DMA *) pcidtf_dev_alloc_dma(PCIDTF_DEV * dev, int len)
{
	PCIDTF_DMA *dma = NULL;
	PCIDTF_DMA_INFO req;

	req.len = len;
	if (xpcf_udev_ioctl(dev->udev, IOCTL_PCIDTF_ALLOC_DMA, &req,
			    sizeof(req), NULL) == 0) {
		dma = pcidtf_dev_add_dma(dev, req.id, len, req.addr);
	}
	return dma;
}

PCIDTF_API_IMP(PCIDTF_DMA *) pcidtf_dev_get_dma(PCIDTF_DEV * dev, int id)
{
	PCIDTF_DMA *dma;
	PCIDTF_DMA_INFO req;

	for (dma = dev->dma; dma; dma = dma->next) {
		if (dma->id == id)
			break;
	}
	if (dma == NULL) {
		req.id = id;
		if (xpcf_udev_ioctl(dev->udev, IOCTL_PCIDTF_GET_DMA_INFO,
				    &req, sizeof(req), NULL) == 0) {
			dma = pcidtf_dev_add_dma(dev, id, req.len, req.addr);
		}
	}
	return dma;
}

PCIDTF_API_IMP(int)pcidtf_dma_get_id(PCIDTF_DMA * dma)
{
	return dma->id;
}

PCIDTF_API_IMP(int)pcidtf_dma_get_len(PCIDTF_DMA * dma)
{
	return dma->len;
}

PCIDTF_API_IMP(UINT64) pcidtf_dma_get_addr(PCIDTF_DMA * dma)
{
	return dma->addr;
}

PCIDTF_API_IMP(void)pcidtf_dma_free(PCIDTF_DMA * dma)
{
	if (xpcf_udev_ioctl(dma->dev->udev, IOCTL_PCIDTF_FREE_DMA,
			    &dma->id, sizeof(dma->id), NULL) == 0)
		free(dma);
}

PCIDTF_API_IMP(int) pcidtf_dma_read(PCIDTF_DMA * dma, int off, void *buf,
				    int len)
{
	PCIDTF_DMA_DATA req;

	req.id = dma->id;
	req.off = off;
	req.len = len;
	req.buf = buf;
	return xpcf_udev_ioctl(dma->dev->udev, IOCTL_PCIDTF_READ_DMA, &req,
			       sizeof(req), NULL);
}

PCIDTF_API_IMP(int) pcidtf_dma_write(PCIDTF_DMA * dma, int off, void *buf,
				     int len)
{
	PCIDTF_DMA_DATA req;

	req.id = dma->id;
	req.off = off;
	req.len = len;
	req.buf = buf;
	return xpcf_udev_ioctl(dma->dev->udev, IOCTL_PCIDTF_WRITE_DMA, &req,
			       sizeof(req), NULL);
}

/* Implement local function */

static PCIDTF_DMA *pcidtf_dev_add_dma(PCIDTF_DEV * dev, int id, int len,
				      unsigned long long addr)
{
	PCIDTF_DMA *dma;

	dma = (PCIDTF_DMA *) malloc(sizeof(PCIDTF_DMA));
	if (dma != NULL) {
		dma->dev = dev;
		dma->id = id;
		dma->len = len;
		dma->addr = addr;
		dma->next = dev->dma;
		dev->dma = dma;
	}
	return dma;
}
