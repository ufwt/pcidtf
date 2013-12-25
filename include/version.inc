# ===================================================================
# Copyright (C) 2013 Hiromitsu Sakamoto
# PCI Device Test Framework
# Version information
# ===================================================================

MAJOR       = 1
MINOR       = 0
BUILD       = 0
BUILD_QFE   = 0

VERINFO=\
    -DPRODUCT_MAJOR_VERSION=$(MAJOR)\
    -DPRODUCT_MINOR_VERSION=$(MINOR)\
    -DPRODUCT_BUILD_VERSION=$(BUILD)\
    -DPRODUCT_BUILD_QFE=$(BUILD_QFE)
