# ===================================================================
# Copyright (C) 2013 Hiromitsu Sakamoto
# PCI Device Test Framework
# Build script for WDM-based kernel-mode driver.
# ===================================================================

STAMP = stampinf -f $@ -a $(_BUILDARCH)\
        -v $(MAJOR).$(MINOR).$(BUILD).$(BUILD_QFE)

$(OBJ_PATH)\$(O)\$(TARGETNAME).inf: $(TARGETNAME).inx 
        copy $(@B).inx $@
        $(STAMP)
