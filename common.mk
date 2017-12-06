ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifndef COMMON_MK_ONCE

all:

COMMON_MK_ONCE = parsed
ROOT ?= $(CURDIR)
LIBNDS  :=      $(DEVKITPRO)/libnds

OUTPUT=$(ROOT)/outnds
O=$(OUTPUT)
EFSTOOL = $(ROOT)/efs
LIBNTXM = $(ROOT)/libntxm
LIBGEDS = $(ROOT)/libgeds
LIBPPP9 = $(ROOT)/ppp9
LIBPLUGINS = $(ROOT)/libplugins
PPP7 = $(OUTPUT)/lib/ppp7.arm7
#NTXM_LIB = $(ROOT)/libntxm/lib
#---------------------------------------------------------------------------------
# path to tools
#---------------------------------------------------------------------------------
export PORTLIBS	:=	$(DEVKITPRO)/portlibs/arm
export PATH	:=	$(DEVKITARM)/bin:$(PORTLIBS)/bin:$(PATH)

#---------------------------------------------------------------------------------
# the prefix on the compiler executables
#---------------------------------------------------------------------------------
PREFIX		?=	arm-eabi-

export CC	:=	$(PREFIX)gcc
export CXX	:=	$(PREFIX)g++
export AS	:=	$(PREFIX)as
export AR	:=	$(PREFIX)ar
export OBJCOPY	:=	$(PREFIX)objcopy

endif
#---------------------------------------------------------------------------------
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
# DATA is a list of directories containing binary files
# all directories are relative to this makefile
#---------------------------------------------------------------------------------

TARGET := $(ROOT)/$M/$M
DATA := $(addprefix $(ROOT)/$M/,$(DATA))
ifndef $M_SOURCES
S:=$(ROOT)/$M/source
else
S:=$($M_SOURCES)
endif
BUILD := $O/$M
DEPSDIR	:= $(BUILD)

$M_CPU := $(CPU)

#################### ARM 9 ######################
ifeq ($($M_CPU),arm9)
ARCH	:=	-mthumb -mthumb-interwork

# note: arm9tdmi isn't the correct CPU arch, but anything newer and LD
# *insists* it has a FPU or VFP, and it won't take no for an answer!
#CFLAGS	:=	-g -Wall -O2\
#		-mcpu=arm9tdmi -mtune=arm9tdmi -fomit-frame-pointer\
#		-ffast-math \
#		$(ARCH)
CFLAGS	=	-g -Wall -O2\
			-march=armv5te -mtune=arm946e-s -fomit-frame-pointer\
			-ffast-math \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9 $($M_CFLAGS) $(_CFLAGS)
CXXFLAGS	= $(CFLAGS) -fno-rtti $(_CXXFLAGS) $(EXTRAFLAGS) $($M_CXXFLAGS)

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wall -Xlinker -Map -Xlinker $(notdir $*.map)


#################### ARM 7 ######################
else
ifeq ($($M_CPU),arm7)
ARCH	:=	-mthumb-interwork

CFLAGS	=	-g -Wall -O2\
		-mcpu=arm7tdmi -mtune=arm7tdmi -fomit-frame-pointer\
		-ffast-math \
		$(ARCH) $(INCLUDE) -DARM7 -D__USING_NTXM__ $(_CFLAGS)

CXXFLAGS =	$(CFLAGS) -fno-rtti -fno-exceptions -fno-rtti -std=gnu++0x $(_CXXFLAGS)


ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm7.specs -g $(ARCH) -Wl,-Map,$(notdir $*).map
else
$(error "no/unknown cpu '$($M_CPU)' defined for $M")
endif
endif


# $1 is variable name, $@ is in $O/$M
modflags = $($(notdir $(patsubst $O/%,%,$(@D)))_$1)

# $1 is variable name, $< is in $(ROOT)/$M
modfiles = $($(notdir $(patsubst $(ROOT)/%,%,$(<D)))_$1)
$M_S := $S

CFILES		:=	$(foreach dir,$S,$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$S,$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$S,$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

OFILES	:=	$(patsubst %,$O/$M/%,$(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o))
DEPENDS	:=	$(OFILES:.o=.o.d)

INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(ROOT)/$M/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include)

LIBDEPS := $(filter %.a,$(patsubst $O/lib/liblib%,$O/lib/lib%,$(patsubst $(ROOT)/%,$O/lib/lib%.a,$(LIBDIRS))))

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

# capture variables into per-module vars.
$M_TARGET := $(TARGET)
$M_BUILD := $(BUILD)
$M_LIBDEPS := $(LIBDEPS)
$M_CFLAGS := $(CFLAGS)
$M_CXXFLAGS := $(CXXFLAGS)
$M_LDFLAGS := $(LDFLAGS)
$M_LIBS := $(LIBS)
$M_LIBDIRS := $(LIBDIRS)
$M_LIBPATHS := $(LIBPATHS)
$M_CPPFILES := $(CPPFILES)
$M_OFILES := $(OFILES)

# clean arguments vars.
CPU:=
_CXXFLAGS :=
_CFLAGS :=

ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro)")
endif

#ISVC=$(or $(VCBUILDHELPER_COMMAND),$(MSBUILDEXTENSIONSPATH32),$(MSBUILDEXTENSIONSPATH))

#ifneq (,$(ISVC))
#	ERROR_FILTER	:=	2>&1 | sed -e 's/\(.[a-zA-Z]\+\):\([0-9]\+\):/\1(\2):/g'
#endif

#---------------------------------------------------------------------------------
%.a:
#---------------------------------------------------------------------------------
	@echo $(notdir $@)
	@mkdir -p $(@D)
	@rm -f $@
	$(AR) -rc $@ $^


#---------------------------------------------------------------------------------
$O/$M/%.arm.o: $S/%.arm.cpp
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $@.arm.d $(call modflags,CXXFLAGS) -marm -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.arm.o: $S/%.arm.c
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.arm.d $(CFLAGS) -marm -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.arm.o: $S/%.arm.m
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.arm.d $(OBJCFLAGS) -marm -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.thumb.o: $S/%.thumb.cpp
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $@.thumb.d $(CXXFLAGS) -mthumb -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.thumb.o: $S/%.thumb.c
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.thumb.d $(CFLAGS) -mthumb -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.thumb.o: $S/%.thumb.m
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.thumb.d $(OBJCFLAGS) -mthumb -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.iwram.o: $S/%.iwram.cpp
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $@.iwram.d $(CXXFLAGS) -marm -mlong-calls -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.iwram.o: $S/%.iwram.c
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.iwram.d $(CFLAGS) -marm -mlong-calls -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.iwram.o: $S/%.iwram.m
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.iwram.d $(OBJCFLAGS) -marm -mlong-calls -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.itcm.o: $S/%.itcm.cpp
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $@.itcm.d $(CXXFLAGS) -marm -mlong-calls -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.itcm.o: $S/%.itcm.c
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.itcm.d $(CFLAGS) -marm -mlong-calls -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.itcm.o: $S/%.itcm.m
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.itcm.d $(OBJCFLAGS) -marm -mlong-calls -c $< -o $@ $(ERROR_FILTER)


#---------------------------------------------------------------------------------
$O/$M/%.o: $S/%.cpp
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CXX) -MMD -MP -MF $@.d $(call modflags,CXXFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.o: $S/%.c
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.d $(call modflags,CFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.o: $S/%.m
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.d $(call modflags,OBJCFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.o: $S/%.s
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.d -x assembler-with-cpp $(call modflags,ASFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
$O/$M/%.o: $S/%.S
	@echo $(notdir $<)
	@mkdir -p $(dir $@)
	$(CC) -MMD -MP -MF $@.d -x assembler-with-cpp $(call modflags,ASFLAGS) -c $< -o $@ $(ERROR_FILTER)

#---------------------------------------------------------------------------------
# canned command sequence for binary data
#---------------------------------------------------------------------------------
define bin2o
	bin2s $< | $(AS) -o $(@)
	echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(<F) | tr . _)`.h
	echo "extern const u8" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(<F) | tr . _)`.h
	echo "extern const u32" `(echo $(<F) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(<F) | tr . _)`.h
endef

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data 
#---------------------------------------------------------------------------------
$O/$M/%.bin.o	: $(DATA)/%.bin
#---------------------------------------------------------------------------------
	mkdir -p $(@D)
	@echo $(notdir $<)
	@$(bin2o)


ifndef V
Q ?= @
endif

ifeq ($(strip $(GAME_TITLE)),)
$M_GAME_TITLE      :=      $M
else
$M_GAME_TITLE := $(GAME_TITLE)
endif

ifeq ($(strip $(GAME_SUBTITLE1)),)
$M_GAME_SUBTITLE1  :=      by PypeBros
else
$M_GAME_SUBTITLE1 := $(GAME_SUBTITLE1)
endif

ifeq ($(strip $(GAME_SUBTITLE2)),)
$M_GAME_SUBTITLE2  :=      sf.net/p/dsgametools
else
$M_GAME_SUBTITLE2 := $(GAME_SUBTITLE2)
endif

ifeq ($(strip $(GAME_ICON)),)
$M_GAME_ICON      :=      $(DEVKITPRO)/libnds/icon.bmp
else
$M_GAME_ICON := $(GAME_ICON)
endif

ifneq ($(strip $(NITRO_FILES)),)
$M_ADDFILES       :=      -d $(NITRO_FILES)
endif
ifneq ($(strip $(EFS_FILES)),)
$M_ADDFILES	:= 	-d $(EFS_FILES)
$(TARGET).nds: $(EFSTOOL)
endif
GAME_ICON :=
GAME_TITLE :=
GAME_SUBTITLE1 :=
GAME_SUBTITLE2 :=
EFS_FILES :=

#---------------------------------------------------------------------------------
%.nds: %.arm9
	@echo converting $< to $@, $(notdir $(patsubst $(ROOT)/%,%,$(<D)))_FILES
	ndstool -c $@ -9 $(filter %.arm9,$^) -7 $(filter %.arm7,$^) -b $(call modfiles,GAME_ICON) "$(call modfiles,GAME_TITLE);$(call modfiles,GAME_SUBTITLE1);$(call modfiles,GAME_SUBTITLE2)" $(call modfiles,ADDFILES)
	@echo built ... $(notdir $@)
	$(if $(findstring $(EFSTOOL),$^), $(EFSTOOL) $@, echo "no EFS for $@")
ifneq ($(DLDI),)
	dlditool $(DLDI) $(OUTPUT).nds
endif

#---------------------------------------------------------------------------------
%.nds: %.elf
	@echo converting $< to $@, $(notdir $(patsubst $(ROOT)/%,%,$(<D)))_FILES
	ndstool -c $@ -9 $< -b $(call modfiles,GAME_ICON) "$(call modfiles,GAME_TITLE);$(call modfiles,GAME_SUBTITLE1);$(call modfiles,GAME_SUBTITLE2)" $(call modfiles,ADDFILES)
	@echo built ... $(notdir $@)

#---------------------------------------------------------------------------------
%.arm9: %.elf
	$Q$(OBJCOPY) -O binary $< $@
	@echo built ... $(notdir $@)

#---------------------------------------------------------------------------------
%.arm7: %.elf
	$(OBJCOPY) -O binary $< $@
	@echo built ... $(notdir $@)

#---------------------------------------------------------------------------------
$O/$M/%.elf:
	@echo mod-linking $(notdir $@)
	$(LD)  $(call modflags,LDFLAGS) $(filter-out %.a,$^) $(call modflags,LIBPATHS) $(call modflags,LIBS) -o $@

$(ROOT)/$M/%.elf:
	@echo mod-linking $(notdir $@)
	$(LD)  $(call modfiles,LDFLAGS) $(filter-out %.a,$^) $(call modfiles,LIBPATHS) $(call modfiles,LIBS) -o $@

