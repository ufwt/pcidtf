/*
 * PCI Device Test Framework
 * Linux kernel-mode driver.
 * This file defines structure types and function prototypes.
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

#ifndef _PCIDTF_H
#define _PCIDTF_H

typedef struct pcidtf_iomap {
	void __iomem *addr;
	unsigned long start;
	int len;
} pcidtf_iomap_t;

typedef struct pcidtf_dma {
	void *vaddr;
	dma_addr_t paddr;
	int len;
} pcidtf_dma_t;

typedef struct pcidtf_dev {
	struct pci_dev *pdev;
	struct device *cdev;
	pcidtf_iomap_t iomap[6];
	int iomap_count;
	int minor;
	pcidtf_dma_t *dma;
	int dma_count;
} pcidtf_dev_t;

extern long pcidtf_ioctl(struct file *filp, unsigned int cmd,
			 unsigned long arg);

#endif
