#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary files
# all directories are relative to this makefile
#---------------------------------------------------------------------------------
BUILD		:=	build
SOURCES		:=  source/cpu/tms9918a source/cpu/sn76496 source/cpu/m6502 source/cpu/tms9900 source/rpk source 
INCLUDES	:=	include 
DATA		:=	data
GRAPHICS	:=	gfx
BACKGRD		:=  gfx_data

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=

CFLAGS	:=  -Wall -Wno-misleading-indentation -O2 -march=armv5te -mtune=arm946e-s -fomit-frame-pointer -ffast-math $(ARCH) -falign-functions=4 -frename-registers -finline-functions -fpredictive-commoning -floop-interchange -ftree-partial-pre -fno-semantic-interposition

CFLAGS	+=	$(INCLUDE) -DARM9

CXXFLAGS	:=	$(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	$(ARCH) -march=armv5te -mtune=arm946e-s -DSN_UPSHIFT=2

LDFLAGS	=	-specs=ds_arm9.specs $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= -lfat -lnds9 -lmm9
 
#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS)
 
#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
 
export ARM9ELF	:=	$(CURDIR)/$(TARGET).elf
export DEPSDIR := $(CURDIR)/$(BUILD)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
          $(foreach dir,$(BACKGRD),$(CURDIR)/$(dir))
 
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
PNGFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))

OSPECIALS :=  splash.o options.o ti99kbd.o ds99kbd.o pdev_bg0.o debug.o

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(OSPECIALS) $(addsuffix .o,$(BINFILES)) \
					$(PNGFILES:.png=.o) $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o) 
 
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)
 
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)
 
.PHONY: $(BUILD) clean
 
#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
 
#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) *.elf *.nds* *.bin 
 
 
#---------------------------------------------------------------------------------
else
 
#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(ARM9ELF)	:	$(OFILES)
	@echo linking $(notdir $@)
	@$(LD)  $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data 
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
	@echo $(notdir $<)
	@$(bin2o)

%.ROM.o	:	%.ROM
	@echo $(notdir $<)
	@$(bin2o)

%.bmp.o	:	%.bmp
	@echo $(notdir $<)
	@$(bin2o)

%.wav.o	:	%.wav
	@echo $(notdir $<)
	@$(bin2o)

# BG------------------------

options.s options.h : options.png
	grit $^ -o $@  -gt -mrt -mR8 -mLs -gzl -mzl

ti99kbd.s ti99kbd.h : ti99kbd.png
	grit $^ -o $@  -gt -mrt -mR8 -mLs -gzl -mzl

splash.s splash.h : splash.png
	grit $^ -o $@  -gt -mrt -mR8 -mLs -gzl -mzl

ds99kbd.s ds99kbd.h : ds99kbd.png
	grit $^ -o $@  -gt -mrt -mR8 -mLs -gzl -mzl

debug.s debug.h : debug.png
	grit $^ -o $@  -gt -mrt -mR8 -mLs -gzl -mzl

pdev_bg0.s pdev_bg0.h : pdev_bg0.png
	grit $^ -o $@  -gt -mrt -mR8 -mLs -gzl -mzl

-include $(DEPENDS)
 
#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
