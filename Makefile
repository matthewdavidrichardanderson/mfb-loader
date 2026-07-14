# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 matthewdavidrichardanderson

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment")
endif

MFB_ROOT ?= $(CURDIR)
export MFB_ROOT

LIBOGC2_RULES := $(DEVKITPRO)/libogc2/wii_rules
ifeq ($(wildcard $(LIBOGC2_RULES)),)
$(error "Pinned libogc2 is required; build and install references/upstream/libogc2")
endif
LIBFAT_ROOT := $(MFB_ROOT)/references/upstream/libfat
LIBFAT_LIB := $(LIBFAT_ROOT)/libogc2/wii/lib/libfat.a
ifeq ($(wildcard $(LIBFAT_LIB)),)
$(error "Pinned libogc2-compatible libfat is required; build references/upstream/libfat with make wii-release")
endif
include $(LIBOGC2_RULES)

TARGET      := mfb-loader
BUILD       := build
SOURCES     := source
INCLUDES    := include
DATA        :=
LIBS        := -lfat -lwiiuse -lbte -ldi -logc -lm
LIBDIRS     :=

CFLAGS      := -g -O2 -Wall -Wextra -Werror -ffunction-sections -DMFB_LIBOGC2=1 \
               $(MACHDEP) $(INCLUDE)
CXXFLAGS    := $(CFLAGS)
LDFLAGS     := -g $(MACHDEP) -Wl,-Map,$(notdir $@).map,--gc-sections
LD          := $(CC)

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH  := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)
export BUILDDIRS := $(BUILD)
export CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export SFILES := freestanding_blob.S
export OFILES := $(CFILES:.c=.o) $(SFILES:.S=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(LIBOGC_INC) \
                  -I$(LIBFAT_ROOT)/include \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := -L$(LIBFAT_ROOT)/libogc2/wii/lib -L$(LIBOGC_LIB) \
                   $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean package freestanding privacy-check
all: freestanding $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

freestanding:
	@$(MAKE) --no-print-directory -C freestanding
	@touch source/freestanding_blob.S

package: all
	@mkdir -p dist/apps/mfb-loader
	@cp $(TARGET).dol dist/apps/mfb-loader/boot.dol
	@cp hbc/icon.png dist/apps/mfb-loader/icon.png
	@cp hbc/meta.xml dist/apps/mfb-loader/meta.xml
	@$(MAKE) --no-print-directory privacy-check

privacy-check:
	@if grep -R -a -i -n -F -e 'C:/Users/' -e 'C:\Users\' dist; then \
		echo 'error: packaged files contain a Windows user-profile path'; \
		exit 1; \
	fi
	@for name in "$(USERNAME)" "$(USER)"; do \
		if [ -n "$$name" ] && grep -R -a -i -n -F "$$name" dist; then \
			echo 'error: packaged files contain a local account name'; \
			exit 1; \
		fi; \
	done

$(BUILD):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD) $(TARGET).dol $(TARGET).elf dist
else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
-include $(DEPENDS)
endif
