/*
 * PCI Device Test Framework
 * This file defines structures and macros for I/O control requests.
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

#ifndef _PCIDTF_GUID_H
#define _PCIDTF_GUID_H

// {D38F0246-1FEC-4e43-8691-941D4C222298}
DEFINE_GUID(GUID_PCIDTF_DEVICE_INTERFACE_CLASS,
	    0xd38f0246, 0x1fec, 0x4e43, 0x86, 0x91, 0x94, 0x1d, 0x4c, 0x22,
	    0x22, 0x98);

#endif
