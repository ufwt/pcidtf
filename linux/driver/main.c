/*
 * PCI Device Test Framework
 * This file implements functions to initialize or terminate Linux
 * kernel-mode driver.
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <asm/current.h>
#include "pcidtf.h"

MODULE_LICENSE("Dual BSD/GPL");

#define PCIDTF_MAX_DEVS	10

static int pcidtf_major = 0;
static int pcidtf_minor = 0;
static struct cdev pcidtf_cdev;
static struct class *pcidtf_class = NULL;

static struct pcidtf_dev *pcidtf_dev[PCIDTF_MAX_DEVS];

static int pcidtf_open(struct inode *inode, struct file *file)
{
/*
	printk("%s - inode 0x%p, file 0x%p\n", __func__, inode, file);
	printk("%s - major %d, minor %d (pid %d)\n", __func__, imajor(inode),
	       iminor(inode), current->pid);
*/
	file->private_data = pcidtf_dev[iminor(inode)];
	return 0;
}

static int pcidtf_close(struct inode *inode, struct file *file)
{
/*
	printk("%s - inode 0x%p, file 0x%p\n", __func__, inode, file);
	printk("%s - major %d, minor %d (pid %d)\n", __func__, imajor(inode),
	       iminor(inode), current->pid);
*/
	return 0;
}

static struct file_operations pcidtf_fops = {
	.open = pcidtf_open,
	.release = pcidtf_close,
	.unlocked_ioctl = pcidtf_ioctl,
};

static struct pci_device_id pcidtf_id_table[] = {
	{PCI_DEVICE_CLASS(PCI_CLASS_SERIAL_USB_XHCI, ~0)},
	{},
};

static void pcidtf_init_iomap(struct pci_dev *pdev, struct pcidtf_dev *dev)
{
	struct pcidtf_iomap *iomap;
	int bar;

	for (bar = 0, iomap = dev->iomap; bar < 6; bar++) {
		iomap->addr = pci_iomap(pdev, bar, 0);
		if (iomap->addr) {
			iomap->start = pci_resource_start(pdev, bar);
			iomap->len = pci_resource_len(pdev, bar);
			printk
			    ("I/O space mapped (bar %u, addr 0x%p, start 0x%lX, len 0x%X)\n",
			     bar, iomap->addr, iomap->start, iomap->len);
			iomap++;
		}
	}
	dev->iomap_count = iomap - dev->iomap;
	printk("Number of I/O spaces = %u\n", dev->iomap_count);
}

static int pcidtf_init_dev(struct pcidtf_dev *data)
{
	data->cdev =
	    device_create(pcidtf_class, NULL, MKDEV(pcidtf_major, pcidtf_minor),
			  NULL, "pcidtf%d", pcidtf_minor);
	if (data->cdev == NULL) {
		return -1;
	}

	data->minor = pcidtf_minor++;

	return 0;
}

static int pcidtf_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct pcidtf_dev *dev;
	int ret;

	printk("Device initialized - ven %04X, dev %04X\n",
	       pdev->vendor, pdev->device);

	dev = kmalloc(sizeof(struct pcidtf_dev), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;

	memset(dev, 0, sizeof(struct pcidtf_dev));

	dev->dma_count = 8;
	dev->dma = kmalloc(sizeof(pcidtf_dma_t) * dev->dma_count, GFP_KERNEL);
	if (dev->dma == NULL) {
		ret = -ENOMEM;
		goto error;
	}
	memset(dev->dma, 0, sizeof(pcidtf_dma_t) * dev->dma_count);

	ret = pci_enable_device(pdev);
	if (ret)
		goto error;

	printk("bus=%d\n", pdev->bus->number);
	printk("devfn=0x%X (%d, %d)\n", pdev->devfn, PCI_SLOT(pdev->devfn),
	       PCI_FUNC(pdev->devfn));

	dev->pdev = pdev;
	pcidtf_init_iomap(pdev, dev);

	pcidtf_init_dev(dev);

	pci_set_drvdata(pdev, dev);

	pcidtf_dev[dev->minor] = dev;

	return 0;

 error:
	if (dev->dma)
		kfree(dev->dma);
	kfree(dev);
	return ret;
}

static void pcidtf_remove(struct pci_dev *pdev)
{
	pcidtf_dev_t *dev = pci_get_drvdata(pdev);
	pcidtf_iomap_t *iomap;
	pcidtf_dma_t *dma;
	int i;

	printk("Device removed\n");

	device_destroy(pcidtf_class, MKDEV(pcidtf_major, dev->minor));

	for (i = 0, iomap = dev->iomap; i < dev->iomap_count; i++, iomap++) {
		if (iomap->addr) {
			printk
			    ("Unmap I/O space (bar %d, addr 0x%p, len 0x%X)\n",
			     i, iomap->addr, iomap->len);
			pci_iounmap(pdev, iomap->addr);
		}
	}

	if ((dma = dev->dma) != NULL) {
		for (i = 0; i < dev->dma_count; i++, dma++) {
			if (dma->vaddr) {
				printk
				    ("Free DMA buffer (idx %d, len %u, vaddr 0x%p, paddr 0x%llX)\n",
				     i, dma->len, dma->vaddr, dma->paddr);
				pci_free_consistent(pdev, dma->len, dma->vaddr,
						    dma->paddr);
			}
		}
		kfree(dev->dma);
	}

	pci_disable_device(pdev);

	kfree(dev);
}

static struct pci_driver pcidtf_driver = {
	.name = "pcidtf",
	.id_table = pcidtf_id_table,
	.probe = pcidtf_probe,
	.remove = pcidtf_remove
};

static int pcidtf_init(void)
{
	dev_t dev = MKDEV(0, 0);
	int alloc_ret, cdev_err = 0, ret;

	printk("Driver loaded\n");

	alloc_ret = alloc_chrdev_region(&dev, 0, PCIDTF_MAX_DEVS, "pcidtf");
	if (alloc_ret) {
		printk("alloc_chrdev_region failed\n");
		goto error;
	}

	pcidtf_major = MAJOR(dev);

	cdev_init(&pcidtf_cdev, &pcidtf_fops);
	pcidtf_cdev.owner = THIS_MODULE;

	cdev_err = cdev_add(&pcidtf_cdev, dev, PCIDTF_MAX_DEVS);
	if (cdev_err) {
		printk("cdev_add failed\n");
		goto error;
	}

	pcidtf_class = class_create(THIS_MODULE, "pcidtf");
	if (IS_ERR(pcidtf_class)) {
		printk("class_create failed\n");
		goto error;
	}

	ret = pci_register_driver(&pcidtf_driver);
	if (ret) {
		printk("pci_register_driver failed\n");
		goto error;
	}

	return 0;

 error:
	if (!IS_ERR(pcidtf_class))
		class_destroy(pcidtf_class);

	if (cdev_err == 0)
		cdev_del(&pcidtf_cdev);

	if (alloc_ret == 0)
		unregister_chrdev_region(dev, PCIDTF_MAX_DEVS);

	return -1;
}

static void pcidtf_exit(void)
{
	pci_unregister_driver(&pcidtf_driver);

	class_destroy(pcidtf_class);

	cdev_del(&pcidtf_cdev);

	unregister_chrdev_region(MKDEV(pcidtf_major, 0), PCIDTF_MAX_DEVS);

	printk("Driver unloaded\n");
}

module_init(pcidtf_init);
module_exit(pcidtf_exit);
