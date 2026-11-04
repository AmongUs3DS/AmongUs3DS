#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
TARGET      := AmongUs3DS
BUILD       := build
SOURCES     := $(shell find source -type d 2>/dev/null || echo source)
DATA        := data
INCLUDES    := include
ROMFS       := romfs

# CIA, SMDH Metadata, and Tools Setup
APP_TITLE   := Among Us
APP_DESCRIPTION := Among Us: 3DS Edition
APP_AUTHOR  := OmgRod
ICON        := icon.png
BANNER_IMAGE := banner.png
BANNER_AUDIO := audio.wav
RSF         := app.rsf
TOOLS_DIR   := tools
MAKEROM     := $(TOOLS_DIR)/makerom
BANNERTOOL  := $(TOOLS_DIR)/bannertool
3DSTOOL     := $(TOOLS_DIR)/3dstool

#---------------------------------------------------------------------------------
ARCH    :=  -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS  :=  -g -Wall -O2 -mword-relocations \
            -fomit-frame-pointer -ffunction-sections \
            $(ARCH)

CFLAGS  +=  $(INCLUDE) -D__3DS__

CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17

ASFLAGS :=  -g $(ARCH)
LDFLAGS =   -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS    := -lcitro2d -lcitro3d -lctru -lm

#---------------------------------------------------------------------------------
LIBDIRS := $(CTRULIB)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT   :=  $(CURDIR)/$(TARGET)
export TOPDIR   :=  $(CURDIR)

export VPATH    :=  $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
            $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR  :=  $(CURDIR)/$(BUILD)

CFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES    :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
CCFILES     :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cc)))
SFILES      :=  $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

ifeq ($(strip $(CPPFILES)),)
    export LD   :=  $(CC)
else
    export LD   :=  $(CXX)
endif

export OFILES   :=  $(CCFILES:.cc=.o) $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE  :=  $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
            $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
            -I$(CURDIR)/$(BUILD)

export LIBPATHS :=  $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
    icons := $(wildcard *.png)
    ifneq (,$(findstring $(TARGET).png,$(icons)))
        export APP_ICON := $(TOPDIR)/$(TARGET).png
    else
        ifneq (,$(findstring icon.png,$(icons)))
            export APP_ICON := $(TOPDIR)/icon.png
        endif
    endif
else
    export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
    export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
    export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: $(BUILD) clean all cia

all: $(BUILD) $(TARGET).cia

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

romfs.bin: $(ROMFS)
	@echo "Compiling RomFS folder into binary..."
	@$(3DSTOOL) -cvtf romfs $@ --romfs-dir $(ROMFS)

manual.bin: manual/export/Manual.bcma
	@echo "Compiling Manual RomFS partition binary..."
	@mkdir -p build_manual
	@cp manual/export/Manual.bcma build_manual/Manual.bcma
	@$(3DSTOOL) -cvtf romfs $@ --romfs-dir build_manual
	@rm -rf build_manual

manual.cfa: manual.bin
	@echo "Packaging Manual CFA..."
	@$(MAKEROM) -f cfa -o $@ -rsf manual/export/Manual.rsf -target t -romfs $<

$(TARGET).cia: $(BUILD) $(TARGET).smdh banner.bnr romfs.bin manual.cfa
	@echo "Packaging CIA..."
	@$(MAKEROM) -f cia -o $@ -elf $(TARGET).elf -rsf $(RSF) -icon $(TARGET).smdh -banner banner.bnr -romfs romfs.bin -exefslogo -target t -i manual.cfa:1:1

$(TARGET).smdh: $(APP_ICON)
	@echo "Generating SMDH..."
	@$(BANNERTOOL) makesmdh -s "$(APP_TITLE)" -l "$(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i "$(APP_ICON)" -o $@

banner.bnr: $(BANNER_IMAGE) $(BANNER_AUDIO)
	@echo "Generating Banner..."
	@$(BANNERTOOL) makebanner -i $(BANNER_IMAGE) -a $(BANNER_AUDIO) -o $@

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).smdh $(OUTPUT).smdh $(TARGET).elf $(TARGET).cia banner.bnr romfs.bin manual.bin manual.cfa build_manual

else
DEPENDS :=  $(OFILES:.o=.d)

ifeq ($(strip $(NO_SMDH)),)
$(OUTPUT).3dsx  :   $(OUTPUT).elf $(OUTPUT).smdh
else
$(OUTPUT).3dsx  :   $(OUTPUT).elf
endif

$(OUTPUT).elf   :   $(OFILES)

-include $(DEPENDS)
endif