# ===================================================================
# Copyright (C) 2013 Hiromitsu Sakamoto
# PCI Device Test Framework
# Build script for KMDF-based kernel-mode driver.
# ===================================================================

STAMP = stampinf -f $@ -a $(_BUILDARCH)\
        -v $(MAJOR).$(MINOR).$(BUILD).$(BUILD_QFE)\
        -k $(KMDF_VERSION_MAJOR).$(KMDF_VERSION_MINOR)

$(OBJ_PATH)\$(O)\$(TARGETNAME).inf: $(TARGETNAME).inx 
        copy $(@B).inx $@
        $(STAMP)
