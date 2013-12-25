/*
 * PCI Device Test Framework
 * This file defines structures and macros for I/O control requests.
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

#ifndef _PCIDTF_IOCTL_H
#define _PCIDTF_IOCTL_H

#include <xpcf/inttypes.h>
#include <xpcf/ioctl.h>

typedef struct pcidtf_dev_info {
	UINT8 bus;
	UINT8 devfn;
	int reg_count;
} PCIDTF_DEV_INFO;

typedef struct pcidtf_cfg_data {
	int off;
	int len;
	UINT32 val;
} PCIDTF_CFG_DATA;

typedef struct pcidtf_reg_info {
	int bar;
	int len;
	UINT64 addr;
} PCIDTF_REG_INFO;

typedef struct pcidtf_reg_data {
	int bar;
	int off;
	int len;
	UINT64 val;
} PCIDTF_REG_DATA;

typedef struct pcidtf_dma_info {
	int id;
	int len;
	UINT64 addr;
} PCIDTF_DMA_INFO;

typedef struct pcidtf_dma_data {
	int id;
	int off;
	int len;
	void *buf;
} PCIDTF_DMA_DATA;

#define IOC_PCIDTF 'P'

#define IOCTL_PCIDTF_GET_INFO       XPCF_IOR(IOC_PCIDTF, 0, PCIDTF_DEV_INFO)
#define IOCTL_PCIDTF_READ_CFG       XPCF_IOWR(IOC_PCIDTF, 1, PCIDTF_CFG_DATA)
#define IOCTL_PCIDTF_WRITE_CFG      XPCF_IOW(IOC_PCIDTF, 2, PCIDTF_CFG_DATA)
#define IOCTL_PCIDTF_GET_REG        XPCF_IOWR(IOC_PCIDTF, 3, PCIDTF_REG_INFO)
#define IOCTL_PCIDTF_READ_REG       XPCF_IOWR(IOC_PCIDTF, 4, PCIDTF_REG_DATA)
#define IOCTL_PCIDTF_WRITE_REG      XPCF_IOW(IOC_PCIDTF, 5, PCIDTF_REG_DATA)
#define IOCTL_PCIDTF_ALLOC_DMA      XPCF_IOWR(IOC_PCIDTF, 6, PCIDTF_DMA_INFO)
#define IOCTL_PCIDTF_FREE_DMA       XPCF_IOW(IOC_PCIDTF, 7, int)
#define IOCTL_PCIDTF_READ_DMA       XPCF_IOWR(IOC_PCIDTF, 8, PCIDTF_DMA_DATA)
#define IOCTL_PCIDTF_WRITE_DMA      XPCF_IOW(IOC_PCIDTF, 9, PCIDTF_DMA_DATA)
#define IOCTL_PCIDTF_GET_DMA_INFO   XPCF_IOWR(IOC_PCIDTF, 10, PCIDTF_DMA_INFO)

#endif
