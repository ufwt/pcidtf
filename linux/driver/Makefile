# ===================================================================
# Copyright (C) 2013 Hiromitsu Sakamoto
# PCI Device Test Framework
# Makefile for GNU C compiler
# ===================================================================

CFILES	= main.c ioctl.c

EXTRA_CFLAGS	:= -I$(PWD)/../../include\
	-I$(PWD)/../../../miscutil/include\
	-D_ENABLE_TRACE_MSG

obj-m	+= pcidtf.o
pcidtf-objs	:= $(CFILES:.c=.o)

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
