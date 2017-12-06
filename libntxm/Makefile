#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	$(shell basename $(CURDIR))
export TOPDIR		:=	$(CURDIR)


#---------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path in windows
#---------------------------------------------------------------------------------
export PATH		:=	$(DEVKITARM)/bin:$(PATH)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: link
	make -C arm9
	make -C arm7
	@[ -d lib ] || mkdir -p lib
	@cp arm9/lib/$(TARGET)9.a arm7/lib/$(TARGET)7.a lib

link: arm7/source/song.cpp

arm7/source/song.cpp: arm9/source/song.cpp
	ln -s ../../arm9/source/song.cpp arm7/source/song.cpp


#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm7 clean
	@rm -f lib/$(TARGET)9.a lib/$(TARGET)7.a
