/*
 * PCI Device Test Framework
 * User-mode Framework Library
 * This file defines common macros and structure types.
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

#ifndef _PCIDTF_DEF_H
#define _PCIDTF_DEF_H

#include "pcidtf_api.h"
#include "pcidtf_ioctl.h"
#include <xpcf/user/udev.h>

#define MAX_DEV_COUNT 10
#define MAX_BAR_COUNT 6

struct pcidtf {
	PCIDTF_DEV *devs[MAX_DEV_COUNT];
	int count;
};

struct pcidtf_dev {
	XPCF_UDEV *udev;
	UINT8 bus;
	UINT8 devfn;
	PCIDTF_IOMAP *iomap[MAX_BAR_COUNT];
	int iomap_count;
	PCIDTF_DMA *dma;
};

struct pcidtf_iomap {
	PCIDTF_DEV *dev;
	int bar;
	int len;
	unsigned long long addr;
};

struct pcidtf_dma {
	PCIDTF_DMA *next;
	PCIDTF_DEV *dev;
	int id;
	int len;
	unsigned long long addr;
};

#endif
