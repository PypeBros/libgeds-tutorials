#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ROOT?=$(abspath $(CURDIR)/..)
M := Demo
CPU := arm9
DATA := datax
INCLUDES := include $(ROOT)/libplugins/include
LIBS = $(EFSLIB) -L $O/lib -lgeds -lppp9 -lntxm9 -lfat -lnds9 #-ldswifi9 
LIBDIRS = $(LIBNDS) $(LIBGEDS) $(LIBPPP9) $(LIBNTXM)
GAME_ICON	:= $(ROOT)/$M/bilogo.bmp
GAME_TITLE	:=    libgeds demo project
_CXXFLAGS = -D__USING_NODA__
EFSLIB:= -D__USING_NODA__
EFS_FILES := $(ROOT)/$M/efsroot

include $(ROOT)/common.mk

.PHONY: $(BUILD) clean all distrib

all: $M-all
$M-all: $(TARGET).nds

#---------------------------------------------------------------------------------
clean: $M-clean
$M-clean:
	@echo $@ ...
	rm -fr $(Demo_BUILD) $(addprefix $(Demo_TARGET), .elf .nds .arm9 .ds.gba)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
OFILES += $(libplugins_OFILES)

$(TARGET).nds	: $(TARGET).arm9 $(PPP7) $(wildcard $(ROOT)/$M/efsroot/*.cmd)
$(TARGET).arm9	:	$(TARGET).elf
$(TARGET).elf	:	$(OFILES) $(LIBDEPS)

debug:
	@echo "OFILES = $(OFILES)"
	@echo "DATA   = $(DATA)"

distrib:
	zip $(OUTPUT)-src.zip * sources/* efsroot/* include/* lib/*

#---------------------------------------------------------------------------------
%.bin.o	:	../$(DATA)/%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

