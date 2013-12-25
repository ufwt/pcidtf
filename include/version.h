/*
 * PCI Device Test Framework
 * This file defines version resource information of this product.
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

#ifndef _VERSION_H
#define _VERSION_H

/* Product version (if not specified) */
#ifndef PRODUCT_MAJOR_VERSION
#define PRODUCT_MAJOR_VERSION   1
#define PRODUCT_MINOR_VERSION   0
#define PRODUCT_BUILD_VERSION   0
#endif

/* Product description */
#define PRODUCT_NAME            "PCI Device Test Framework"

/* Copyright information */
#define PRODUCT_COPYRIGHT       "(C) 2013 Hiromitsu Sakamoto"

#ifdef WIN32
#include <win/resource.ver>
#endif

#endif
