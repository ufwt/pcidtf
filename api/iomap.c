/*
 * PCI Device Test Framework
 * User-mode Framework Library
 * This file implements I/O register access functions.
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

PCIDTF_API_IMP(int) pcidtf_dev_get_iomap_count(PCIDTF_DEV * dev)
{
	return dev->iomap_count;
}

PCIDTF_API_IMP(PCIDTF_IOMAP *) pcidtf_dev_get_iomap(PCIDTF_DEV * dev, int bar)
{
	if (bar < 0 || bar >= dev->iomap_count)
		return NULL;
	return dev->iomap[bar];
}

PCIDTF_API_IMP(int)pcidtf_iomap_get_bar(PCIDTF_IOMAP * iomap)
{
	return iomap->bar;
}

PCIDTF_API_IMP(int)pcidtf_iomap_get_len(PCIDTF_IOMAP * iomap)
{
	return iomap->len;
}

PCIDTF_API_IMP(UINT64) pcidtf_iomap_get_addr(PCIDTF_IOMAP * iomap)
{
	return iomap->addr;
}

PCIDTF_API_IMP(int)pcidtf_iomap_read_reg(PCIDTF_IOMAP * iomap, int off, int len,
					 UINT64 * val)
{
	PCIDTF_REG_DATA data;
	int ret;

	data.bar = iomap->bar;
	data.off = off;
	data.len = len;
	data.val = 0;
	ret = xpcf_udev_ioctl(iomap->dev->udev, IOCTL_PCIDTF_READ_REG,
			      &data, sizeof(data), NULL);
	if (ret)
		return ret;
	*val = data.val;
	return 0;
}

PCIDTF_API_IMP(int)pcidtf_iomap_write_reg(PCIDTF_IOMAP * iomap, int off,
					  int len, UINT64 val)
{
	PCIDTF_REG_DATA data;

	data.bar = iomap->bar;
	data.off = off;
	data.len = len;
	data.val = val;
	return xpcf_udev_ioctl(iomap->dev->udev, IOCTL_PCIDTF_WRITE_REG,
			       &data, sizeof(data), NULL);
}
