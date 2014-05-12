# ===================================================================
# Copyright (C) 2013 Hiromitsu Sakamoto
# PCI Device Test Framework
# Makefile for GNU C compiler
# ===================================================================

MAKEFILE = makefile.gcc

DIRS	= linux api testapp

all:
	@for dir in $(DIRS); do\
		$(MAKE) -C $$dir -f $(MAKEFILE);\
	done

clean:
	@for dir in $(DIRS); do\
		cd $$dir; $(MAKE) -f $(MAKEFILE) clean; cd ..;\
	done
	@rm -f *~

install:
	@for dir in $(DIRS); do\
		cd $$dir; $(MAKE) -f $(MAKEFILE) install; cd ..;\
	done
