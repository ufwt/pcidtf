PCI Device Test Framework (pcidtf)
==================================

PCI Device Test Framework (PCIDTF) is a set of kernel-mode drivers and
user-mode library that can be used to write test application programs
for various types of PCI bus devices (including PCI express).

The framework provides the following functions to applications:

- Enumeration of target PCI devices.

- Read/write access of PCI configuration space.

- Read/write access of I/O registers, either port I/O or memory-mapped
  I/O.

- Management and read/write access of system memory buffers for DMA
  operation.

Requirements
------------

PCIDTF requires the following software component:

- Miscellaneous Utilities (miscutil)

Target platforms
----------------

PCIDTF can be used for the following platforms:

- Linux
   * Kernel-mode driver and user-mode library can be built by GNU C
     compiler.

- Windows
   * Kernel-mode drivers (WDM and KMDF) can be built by Windows Driver
     Kit for Windows 7.
   * User-mode DLL can be built by Visual Studio 2012 or Windows
     Driver Kit for Windows 7.

----
Copyright (C) 2013-2014 Hiromitsu Sakamoto
