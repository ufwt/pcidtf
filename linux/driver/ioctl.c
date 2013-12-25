/*
 * PCI Device Test Framework
 * This file implements I/O control functions.
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

#include <linux/pci.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

#include "pcidtf.h"
#include "pcidtf_ioctl.h"

long pcidtf_get_info(pcidtf_dev_t * dev, unsigned long arg)
{
	PCIDTF_DEV_INFO data;
	long ret = 0;

	data.bus = dev->pdev->bus->number;
	data.devfn = dev->pdev->devfn;

	if (!access_ok(VERIFY_WRITE, (void __user *)arg, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}
	if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}
 done:
	return ret;
}

long pcidtf_rw_cfg(pcidtf_dev_t * dev, unsigned int cmd, unsigned long arg)
{
	PCIDTF_CFG_DATA data;
	long ret = 0;

	memset(&data, 0, sizeof(data));

	if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}
	printk("IOCTL - cmd=0x%X, off=%d, len=%d\n", cmd, data.off, data.len);

	if (data.off < 0) {
		ret = -EINVAL;
		goto done;
	}
	switch (data.len) {
	case 1:
		if (cmd == IOCTL_PCIDTF_READ_CFG)
			ret = pci_read_config_byte(dev->pdev, data.off,
						   (u8 *) & data.val);
		else
			ret = pci_write_config_byte(dev->pdev, data.off,
						    data.val);
		break;
	case 2:
		if (cmd == IOCTL_PCIDTF_READ_CFG)
			ret = pci_read_config_word(dev->pdev, data.off,
						   (u16 *) & data.val);
		else
			ret = pci_write_config_word(dev->pdev, data.off,
						    data.val);
		break;
	case 4:
		if (cmd == IOCTL_PCIDTF_READ_CFG)
			ret = pci_read_config_dword(dev->pdev, data.off,
						    &data.val);
		else
			ret = pci_write_config_dword(dev->pdev, data.off,
						     data.val);
		break;
	default:
		ret = -EINVAL;
		goto done;
	}
	if (cmd == IOCTL_PCIDTF_WRITE_CFG)
		goto done;
	if (!access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd))) {
		ret = -EFAULT;
		goto done;
	}
	if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}

 done:
	return ret;
}

long pcidtf_get_reg_info(pcidtf_dev_t * dev, unsigned long arg)
{
	PCIDTF_REG_INFO data;
	pcidtf_iomap_t *iomap;
	long ret = 0;

	memset(&data, 0, sizeof(data));

	if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}

	if (data.bar < 0 || data.bar >= dev->iomap_count) {
		ret = -EINVAL;
		goto done;
	}
	iomap = dev->iomap + data.bar;
	data.addr = iomap->start;
	data.len = iomap->len;

	if (!access_ok(VERIFY_WRITE, (void __user *)arg, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}
	if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}
 done:
	return ret;
}

long pcidtf_rw_reg(pcidtf_dev_t * dev, unsigned int cmd, unsigned long arg)
{
	PCIDTF_REG_DATA data;
	pcidtf_iomap_t *iomap;
	void __iomem *addr;
	int ret = 0;

	memset(&data, 0, sizeof(data));

	if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}
	printk("IOCTL - cmd=0x%X, bar=%d, off=%d, len=%d\n", cmd, data.bar,
	       data.off, data.len);

	if (data.bar < 0 || data.bar >= dev->iomap_count) {
		ret = -EINVAL;
		goto done;
	}
	iomap = dev->iomap + data.bar;
	if (data.off < 0 || data.off + data.len >= iomap->len) {
		ret = -EINVAL;
		goto done;
	}
	addr = (unsigned char *)iomap->addr + data.off;

	switch (data.len) {
	case 1:
		if (cmd == IOCTL_PCIDTF_READ_REG)
			data.val = ioread8(addr);
		else
			iowrite8(data.val, addr);
		break;
	case 2:
		if (cmd == IOCTL_PCIDTF_READ_REG)
			data.val = ioread16(addr);
		else
			iowrite16(data.val, addr);
		break;
	case 4:
		if (cmd == IOCTL_PCIDTF_READ_REG)
			data.val = ioread32(addr);
		else
			iowrite32(data.val, addr);
		break;
	default:
		ret = -EINVAL;
		goto done;
		break;
	}

	if (cmd == IOCTL_PCIDTF_WRITE_REG)
		goto done;
	if (!access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd))) {
		ret = -EFAULT;
		goto done;
	}
	if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}

 done:
	return ret;
}

long pcidtf_alloc_dma(pcidtf_dev_t * dev, unsigned long arg)
{
	PCIDTF_DMA_INFO data;
	pcidtf_dma_t *dma = NULL;
	int i, new_count;
	long ret = 0;

	memset(&data, 0, sizeof(data));

	if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}

	for (i = 0; i < dev->dma_count; i++) {
		if (dev->dma[i].vaddr == NULL) {
			dma = &dev->dma[i];
			break;
		}
	}
	if (dma == NULL) {
		new_count = dev->dma_count << 1;
		dma =
		    kmalloc(sizeof(pcidtf_dma_t) * dev->dma_count, GFP_KERNEL);
		if (dma == NULL) {
			ret = -ENOMEM;
			goto done;
		}
		memcpy(dma, dev->dma, sizeof(pcidtf_dma_t) * dev->dma_count);
		kfree(dev->dma);
		dev->dma = dma;
		dma = &dma[dev->dma_count];
		dev->dma_count = new_count;
	}

	dma->vaddr = pci_alloc_consistent(dev->pdev, data.len, &dma->paddr);
	if (dma->vaddr == NULL) {
		ret = -ENOMEM;
		goto done;
	}
	dma->len = data.len;

	printk
	    ("DMA buffer allocated (idx %d, len %u, vaddr 0x%p, paddr 0x%llX)\n",
	     i, dma->len, dma->vaddr, dma->paddr);

	data.id = i + 1;
	data.addr = dma->paddr;

	if (!access_ok(VERIFY_WRITE, (void __user *)arg, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}
	if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}

 done:
	return ret;
}

pcidtf_dma_t *pcidtf_get_dma(pcidtf_dev_t * dev, int id)
{
	pcidtf_dma_t *dma;

	if (dev->dma == NULL || id <= 0 || id > dev->dma_count)
		return NULL;
	dma = &dev->dma[id - 1];
	return (dma->vaddr ? dma : NULL);
}

long pcidtf_free_dma(pcidtf_dev_t * dev, unsigned long arg)
{
	int id = 0;
	pcidtf_dma_t *dma;
	long ret = 0;

	if (copy_from_user(&id, (int __user *)arg, sizeof(id))) {
		ret = -EFAULT;
		goto done;
	}
	dma = pcidtf_get_dma(dev, id);
	if (dma == NULL) {
		ret = -EINVAL;
		goto done;
	}

	printk("Free DMA buffer (idx %d, len %u, vaddr 0x%p, paddr 0x%llX)\n",
	       id - 1, dma->len, dma->vaddr, dma->paddr);
	pci_free_consistent(dev->pdev, dma->len, dma->vaddr, dma->paddr);
	dma->vaddr = NULL;

 done:
	return ret;
}

long pcidtf_rw_dma(pcidtf_dev_t * dev, unsigned int cmd, unsigned long arg)
{
	PCIDTF_DMA_DATA data;
	pcidtf_dma_t *dma;
	unsigned char *bp;
	long ret = 0;

	memset(&data, 0, sizeof(data));

	if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
		ret = -EFAULT;
		goto done;
	}

	printk("id=%d, off=%d, len=%d\n", data.id, data.off, data.len);

	dma = pcidtf_get_dma(dev, data.id);
	if (dma == NULL) {
		ret = -EINVAL;
		goto done;
	}

	if (data.off + data.len > dma->len) {
		printk("Invalid param - off %d, len %d (%u)\n", data.off,
		       data.len, dma->len);
		ret = -EINVAL;
		goto done;
	}

	bp = (unsigned char *)dma->vaddr + data.off;
	if (cmd == IOCTL_PCIDTF_READ_DMA) {
		if (!access_ok(VERIFY_READ, data.buf, data.len)) {
			ret = -EFAULT;
			goto done;
		}
		if (copy_to_user(data.buf, bp, data.len)) {
			ret = -EFAULT;
			goto done;
		}
	} else {
		if (!access_ok(VERIFY_WRITE, data.buf, data.len)) {
			ret = -EFAULT;
			goto done;
		}
		if (copy_from_user(bp, data.buf, data.len)) {
			ret = -EFAULT;
			goto done;
		}
	}
 done:
	return ret;
}

long pcidtf_get_dma_info(pcidtf_dev_t * dev, unsigned long arg)
{
	PCIDTF_DMA_INFO req;
	pcidtf_dma_t *dma;
	long ret = 0;

	if (copy_from_user(&req, (int __user *)arg, sizeof(req))) {
		ret = -EFAULT;
		goto done;
	}
	dma = pcidtf_get_dma(dev, req.id);
	if (dma == NULL) {
		ret = -EINVAL;
		goto done;
	}
	req.len = dma->len;
	req.addr = dma->paddr;
	if (!access_ok(VERIFY_WRITE, (void __user *)arg, sizeof(req))) {
		ret = -EFAULT;
		goto done;
	}
	if (copy_to_user((int __user *)arg, &req, sizeof(req))) {
		ret = -EFAULT;
		goto done;
	}
 done:
	return ret;
}

long pcidtf_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	pcidtf_dev_t *dev = filp->private_data;
	long ret = 0;

	if (cmd == IOCTL_PCIDTF_GET_INFO) {
		ret = pcidtf_get_info(dev, arg);
		goto done;
	}

	if (!access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd))) {
		ret = -EFAULT;
		goto done;
	}

	switch (cmd) {
	case IOCTL_PCIDTF_READ_CFG:
	case IOCTL_PCIDTF_WRITE_CFG:
		ret = pcidtf_rw_cfg(dev, cmd, arg);
		break;
	case IOCTL_PCIDTF_GET_REG:
		ret = pcidtf_get_reg_info(dev, arg);
		break;
	case IOCTL_PCIDTF_READ_REG:
	case IOCTL_PCIDTF_WRITE_REG:
		ret = pcidtf_rw_reg(dev, cmd, arg);
		break;
	case IOCTL_PCIDTF_ALLOC_DMA:
		ret = pcidtf_alloc_dma(dev, arg);
		break;
	case IOCTL_PCIDTF_FREE_DMA:
		ret = pcidtf_free_dma(dev, arg);
		break;
	case IOCTL_PCIDTF_READ_DMA:
	case IOCTL_PCIDTF_WRITE_DMA:
		ret = pcidtf_rw_dma(dev, cmd, arg);
		break;
	case IOCTL_PCIDTF_GET_DMA_INFO:
		ret = pcidtf_get_dma_info(dev, arg);
		break;
	default:
		ret = -ENOTTY;
		break;
	}

 done:
	return ret;
}
