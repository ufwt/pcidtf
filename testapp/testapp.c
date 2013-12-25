/*
 * PCI Device Test Framework
 * Simple applications program
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

#include <pcidtf_api.h>
#include <xpcf/string.h>
#include <xpcf/dumpmem.h>
#include <xpcf/user/getparam.h>
#include <version.h>

#define APP_NAME "pcidtf_testapp"

static void show_app_info(PCIDTF * dtf)
{
	int count = pcidtf_get_dev_count(dtf);

	fprintf(stderr,
		"PCI Device Test Framework Test Program Version %d.%d.%d\n",
		PRODUCT_MAJOR_VERSION, PRODUCT_MINOR_VERSION,
		PRODUCT_BUILD_VERSION);
	fprintf(stderr, "Copyright " PRODUCT_COPYRIGHT "\n");
	if (count == 0) {
		fprintf(stderr, "No device found\n\n");
	} else {
		fprintf(stderr, "%d device%s found\n\n", count,
			count == 1 ? "" : "(s)");
	}
}

void dev_cmd(PCIDTF * dtf, int argc, char *argv[])
{
	enum {
		CMD_INFO
	} cmd;
	PCIDTF_DEV *dev;
	int params[1];

	if (argc == 4 && strcasecmp(argv[2], "info") == 0) {
		cmd = CMD_INFO;
	} else {
		show_app_info(dtf);
		fprintf(stderr, "Usage: " APP_NAME " dev info <idx>\n");
		exit(1);
	}
	xpcf_get_int_params(argc - 3, argv + 3, params);
	if ((dev = pcidtf_get_dev(dtf, params[0])) == NULL) {
		fprintf(stderr, "ERROR: invalid idx=%d\n", params[0]);
		exit(1);
	}
	if (cmd == CMD_INFO) {
		printf("bus=%u, devfn=%u\n", pcidtf_dev_get_bus(dev),
		       pcidtf_dev_get_devfn(dev));
	}
}

void cfg_cmd(PCIDTF * dtf, int argc, char *argv[])
{
	enum {
		CMD_READ,
		CMD_WRITE
	} cmd;
	PCIDTF_DEV *dev;
	int params[4];
	unsigned int val;

	if (argc == 6 && strcasecmp(argv[2], "read") == 0) {
		cmd = CMD_READ;
	} else if (argc == 7 && strcasecmp(argv[2], "write") == 0) {
		cmd = CMD_WRITE;
	} else {
		show_app_info(dtf);
		fprintf(stderr,
			"Usage: " APP_NAME " cfg read <idx> <off> <len>\n");
		fprintf(stderr,
			"       " APP_NAME
			" cfg write <idx> <off> <len> <val>\n");
		exit(1);
	}
	xpcf_get_int_params(argc - 3, argv + 3, params);
	if ((dev = pcidtf_get_dev(dtf, params[0])) == NULL) {
		fprintf(stderr, "ERROR: invalid idx=%d\n", params[0]);
		exit(1);
	}
	if (cmd == CMD_READ) {
		if (pcidtf_dev_read_cfg(dev, params[1], params[2], &val) < 0) {
			fprintf(stderr, "ERROR: failed to read PCI config\n");
			exit(1);
		}
		printf("PCI config read - off=%d, len=%d, val=0x%X\n",
		       params[1], params[2], val);
	} else {
		if (pcidtf_dev_write_cfg(dev, params[1], params[2], params[3]) <
		    0) {
			fprintf(stderr, "ERROR: failed to write PCI config\n");
			exit(1);
		}
		printf("PCI config written - off=%d, len=%d, val=0x%X\n",
		       params[1], params[2], params[3]);
	}
}

void reg_cmd(PCIDTF * dtf, int argc, char *argv[])
{
	enum {
		CMD_INFO,
		CMD_READ,
		CMD_WRITE
	} cmd;
	PCIDTF_DEV *dev;
	PCIDTF_IOMAP *iomap;
	int params[5];
	UINT64 val;

	if (argc == 5 && strcasecmp(argv[2], "info") == 0) {
		cmd = CMD_INFO;
	} else if (argc == 7 && strcasecmp(argv[2], "read") == 0) {
		cmd = CMD_READ;
	} else if (argc == 8 && strcasecmp(argv[2], "write") == 0) {
		cmd = CMD_WRITE;
	} else {
		show_app_info(dtf);
		fprintf(stderr, "Usage: " APP_NAME " reg info <idx> <bar>\n");
		fprintf(stderr,
			"       " APP_NAME
			" reg read <idx> <bar> <off> <len>\n");
		fprintf(stderr,
			"       " APP_NAME
			" reg write <idx> <bar> <off> <len> <val>\n");
		exit(1);
	}
	xpcf_get_int_params(argc - 3, argv + 3, params);
	if ((dev = pcidtf_get_dev(dtf, params[0])) == NULL) {
		fprintf(stderr, "ERROR: invalid idx=%d\n", params[0]);
		exit(1);
	}
	if ((iomap = pcidtf_dev_get_iomap(dev, params[1])) == NULL) {
		fprintf(stderr, "ERROR: invalid bar=%d\n", params[1]);
		exit(1);
	}
	if (cmd == CMD_INFO) {
		printf("Register info - bar %d, len=%d, addr=0x%llX\n",
		       params[1], pcidtf_iomap_get_len(iomap),
		       pcidtf_iomap_get_addr(iomap));
	} else if (cmd == CMD_READ) {
		if (pcidtf_iomap_read_reg(iomap, params[2], params[3], &val)) {
			fprintf(stderr, "ERROR: failed to read I/O register\n");
			exit(1);
		}
		printf("Register read - bar=%d, off=%d, len=%d, val=0x%llX\n",
		       params[1], params[2], params[3], val);
	} else {
		if (pcidtf_iomap_write_reg
		    (iomap, params[2], params[3], params[4])) {
			fprintf(stderr,
				"ERROR: failed to write I/O register\n");
			exit(1);
		}
		printf("Register written - bar=%d, off=%d, len=%d, val=0x%X\n",
		       params[1], params[2], params[3], params[4]);
	}
}

void dma_cmd(PCIDTF * dtf, int argc, char *argv[])
{
	enum {
		CMD_ALLOC,
		CMD_FREE,
		CMD_READ,
		CMD_WRITE,
		CMD_INFO
	} cmd;
	PCIDTF_DEV *dev;
	PCIDTF_DMA *dma;
	int params[5];
	unsigned char *buf;

	if (argc == 5 && strcasecmp(argv[2], "alloc") == 0) {
		cmd = CMD_ALLOC;
	} else if (argc == 5 && strcasecmp(argv[2], "free") == 0) {
		cmd = CMD_FREE;
	} else if (argc == 7 && strcasecmp(argv[2], "read") == 0) {
		cmd = CMD_READ;
	} else if (argc == 8 && strcasecmp(argv[2], "write") == 0) {
		cmd = CMD_WRITE;
	} else if (argc == 5 && strcasecmp(argv[2], "info") == 0) {
		cmd = CMD_INFO;
	} else {
		show_app_info(dtf);
		fprintf(stderr, "Usage: " APP_NAME " dma alloc <idx> <len>\n");
		fprintf(stderr, "       " APP_NAME " dma free <idx> <id>\n");
		fprintf(stderr,
			"       " APP_NAME
			" dma read <idx> <id> <off> <len>\n");
		fprintf(stderr,
			"       " APP_NAME
			" dma write <idx> <id> <off> <len> <val>\n");
		fprintf(stderr, "       " APP_NAME " dma info <idx> <id>\n");
		exit(1);
	}
	xpcf_get_int_params(argc - 3, argv + 3, params);
	if ((dev = pcidtf_get_dev(dtf, params[0])) == NULL) {
		fprintf(stderr, "ERROR: invalid idx=%d\n", params[0]);
		exit(1);
	}
	if (cmd == CMD_ALLOC) {
		if ((dma = pcidtf_dev_alloc_dma(dev, params[1])) == NULL) {
			fprintf(stderr,
				"ERROR: failed to allocate DMA buffer\n");
			exit(1);
		}
		printf("DMA buffer allocated - id=%d, len=%d, addr=0x%llX\n",
		       pcidtf_dma_get_id(dma), params[1],
		       pcidtf_dma_get_addr(dma));
	} else {
		if ((dma = pcidtf_dev_get_dma(dev, params[1])) == NULL) {
			fprintf(stderr, "ERROR: invalid id=%d\n", params[1]);
			exit(1);
		}
		if (cmd == CMD_FREE) {
			pcidtf_dma_free(dma);
			printf("DMA buffer freed - id=%d\n", params[1]);
		} else if (cmd == CMD_READ) {
			if ((buf = (unsigned char *)malloc(params[3])) == NULL) {
				perror("malloc");
				exit(1);
			}
			if (pcidtf_dma_read(dma, params[2], buf, params[3]) < 0) {
				fprintf(stderr,
					"ERROR: failed to read DMA buffer\n");
				exit(1);
			}
			printf("DMA buffer read - id=%d, off=%d, len=%d\n",
			       params[1], params[2], params[3]);
			dump_mem(buf, params[3]);
			free(buf);
		} else if (cmd == CMD_WRITE) {
			if (pcidtf_dma_write(dma, params[2], &params[4],
					     params[3]) < 0) {
				fprintf(stderr,
					"ERROR: failed to write DMA buffer\n");
				exit(1);
			}
			printf
			    ("DMA buffer written - id=%d, off=%d, len=%d, val=0x%X\n",
			     params[1], params[2], params[3], params[4]);
		} else {
			printf("DMA buffer - id=%d, len=%d, addr=0x%llX\n",
			       params[1], pcidtf_dma_get_len(dma),
			       pcidtf_dma_get_addr(dma));
		}
	}
}

#ifdef WIN32
int __cdecl
#else
int
#endif
main(int argc, char *argv[])
{
	PCIDTF *dtf;

	if ((dtf = pcidtf_init()) == NULL) {
		fprintf(stderr, "ERROR: pcidtf_init failed\n");
		exit(1);
	}
	if (argc >= 2 && strcasecmp(argv[1], "dev") == 0) {
		dev_cmd(dtf, argc, argv);
	} else if (argc >= 2 && strcasecmp(argv[1], "cfg") == 0) {
		cfg_cmd(dtf, argc, argv);
	} else if (argc >= 2 && strcasecmp(argv[1], "reg") == 0) {
		reg_cmd(dtf, argc, argv);
	} else if (argc >= 2 && strcasecmp(argv[1], "dma") == 0) {
		dma_cmd(dtf, argc, argv);
	} else {
		show_app_info(dtf);
		fprintf(stderr, "Usage: " APP_NAME " dev\n");
		fprintf(stderr, "       " APP_NAME " cfg\n");
		fprintf(stderr, "       " APP_NAME " reg\n");
		fprintf(stderr, "       " APP_NAME " dma\n");
		exit(1);
	}
	pcidtf_cleanup(dtf);
	return 0;
}
