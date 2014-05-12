/*
 * PCI Device Test Framework
 * User-mode Framework Library
 * This file implements global and device level API functions.
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

#include "pcidtf_def.h"
#include <xpcf/status.h>
#ifdef WIN32
#include <win/user/devenum.h>
#include <initguid.h>
#include <pcidtf_guid.h>
#else
#include <string.h>
#endif

/* Local function prototypes */
static void pcidtf_dev_free(PCIDTF_DEV * dev);
static int pcidtf_enum(PCIDTF * data);

XPCF_API_IMP(PCIDTF *) pcidtf_init(void)
{
	PCIDTF *dtf;

	dtf = (PCIDTF *) malloc(sizeof(PCIDTF));
	if (dtf != NULL) {
		memset(dtf, 0, sizeof(PCIDTF));
		if (pcidtf_enum(dtf)) {
			free(dtf);
			dtf = NULL;
		}
	}
	return dtf;
}

XPCF_API_IMP(void)pcidtf_cleanup(PCIDTF * dtf)
{
	PCIDTF_DEV *dev;
	int i;

	for (i = 0; i < dtf->count; i++) {
		if ((dev = dtf->devs[i]) == NULL)
			break;
		pcidtf_dev_free(dev);
	}
	free(dtf);
}

XPCF_API_IMP(int) pcidtf_get_dev_count(PCIDTF * dtf)
{
	return dtf->count;
}

XPCF_API_IMP(PCIDTF_DEV *) pcidtf_get_dev(PCIDTF * dtf, int idx)
{
	PCIDTF_DEV *dev = NULL;

	if (idx >= 0 && idx < dtf->count)
		dev = dtf->devs[idx];
	return dev;
}

XPCF_API_IMP(UINT8) pcidtf_dev_get_bus(PCIDTF_DEV * dev)
{
	return dev->bus;
}

XPCF_API_IMP(UINT8) pcidtf_dev_get_devfn(PCIDTF_DEV * dev)
{
	return dev->devfn;
}

XPCF_API_IMP(int)pcidtf_dev_read_cfg(PCIDTF_DEV * dev, int off, int len,
				     UINT32 * val)
{
	PCIDTF_CFG_DATA data;
	int ret;

	data.off = off;
	data.len = len;
	data.val = 0;
	if ((ret = xpcf_udev_ioctl(dev->udev, IOCTL_PCIDTF_READ_CFG,
				   &data, sizeof(data), NULL)) < 0)
		return ret;
	*val = data.val;
	return 0;
}

XPCF_API_IMP(int)pcidtf_dev_write_cfg(PCIDTF_DEV * dev, int off, int len,
				      UINT32 val)
{
	PCIDTF_CFG_DATA data;

	data.off = off;
	data.len = len;
	data.val = val;
	return xpcf_udev_ioctl(dev->udev, IOCTL_PCIDTF_WRITE_CFG, &data,
			       sizeof(data), NULL);
}

/* Implement local functions */

static int pcidtf_enum_iomap(PCIDTF_DEV * dev)
{
	PCIDTF_REG_INFO req;
	PCIDTF_IOMAP *iomap;
	int bar;

	for (bar = 0; bar < dev->iomap_count; bar++) {
		req.bar = bar;
		if (xpcf_udev_ioctl(dev->udev, IOCTL_PCIDTF_GET_REG,
				    &req, sizeof(req), NULL) < 0)
			break;
		iomap = (PCIDTF_IOMAP *) malloc(sizeof(PCIDTF_IOMAP));
		if (iomap == NULL)
			return XPCF_STS_MEM_ALLOC_ERR;
		iomap->dev = dev;
		iomap->bar = bar;
		iomap->len = req.len;
		iomap->addr = req.addr;
		dev->iomap[bar] = iomap;
	}
	return 0;
}

static void pcidtf_dev_free(PCIDTF_DEV * dev)
{
	int i;

	for (i = 0; i < dev->iomap_count; i++)
		free(dev->iomap[i]);
	if (dev->udev)
		xpcf_udev_close(dev->udev);
	free(dev);
}

#ifdef WIN32
static void enum_handler(void *ctx,
			 HDEVINFO hDevInfo,
			 PSP_DEVINFO_DATA pDevInfoData,
			 PSP_DEVICE_INTERFACE_DETAIL_DATA pDevIntfDetailData)
{
	PCIDTF *data = (PCIDTF *) ctx;
	XPCF_UDEV *udev;
	PCIDTF_DEV_INFO req;
	PCIDTF_DEV *dev;

	UNREFERENCED_PARAMETER(hDevInfo);
	UNREFERENCED_PARAMETER(pDevInfoData);

	if (xpcf_udev_open(pDevIntfDetailData->DevicePath, &udev) == 0) {
		if (xpcf_udev_ioctl(udev, IOCTL_PCIDTF_GET_INFO,
				    &req, sizeof(req), NULL)) {
			xpcf_udev_close(udev);
		} else if ((dev = (PCIDTF_DEV *) malloc(sizeof(PCIDTF_DEV))) ==
			   NULL) {
			xpcf_udev_close(udev);
		} else {
			memset(dev, 0, sizeof(PCIDTF_DEV));
			dev->udev = udev;
			dev->bus = req.bus;
			dev->devfn = req.devfn;
			if (req.reg_count > MAX_BAR_COUNT) {
				pcidtf_dev_free(dev);
			} else {
				dev->iomap_count = req.reg_count;
				if (pcidtf_enum_iomap(dev)) {
					pcidtf_dev_free(dev);
				} else {
					data->devs[data->count++] = dev;
				}
			}
		}
	}
}
#endif

static int pcidtf_enum(PCIDTF * data)
{
#ifdef WIN32
	return EnumDevNode(&GUID_PCIDTF_DEVICE_INTERFACE_CLASS, enum_handler,
			   data);
#else
	XPCF_UDEV *udev;
	PCIDTF_DEV *dev;
	PCIDTF_DEV_INFO req;
	char name[16];
	int idx, ret = 0;

	data->count = 0;
	for (idx = 0; idx < MAX_DEV_COUNT; idx++) {
		snprintf(name, sizeof(name), "/dev/pcidtf%d", idx);
		if (xpcf_udev_open(name, &udev))
			continue;
		ret = xpcf_udev_ioctl(udev, IOCTL_PCIDTF_GET_INFO,
				      &req, sizeof(req), NULL);
		if (ret) {
			xpcf_udev_close(udev);
			break;
		}
		if ((dev = (PCIDTF_DEV *) malloc(sizeof(PCIDTF_DEV))) == NULL) {
			xpcf_udev_close(udev);
			ret = XPCF_STS_MEM_ALLOC_ERR;
			break;
		}
		memset(dev, 0, sizeof(PCIDTF_DEV));
		dev->udev = udev;
		dev->bus = req.bus;
		dev->devfn = req.devfn;
		ret = pcidtf_enum_iomap(dev);
		if (ret) {
			pcidtf_dev_free(dev);
			break;
		}
		data->devs[data->count++] = dev;
	}
	return ret;
#endif
}
