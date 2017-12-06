#---------------------------------------------------------------------------------
.SUFFIXES:

ROOT ?= $(abspath $(CURDIR)/..)
M 	:= ppp9
CPU	:= arm9
TARGET 	:= $M
DATA	:= data
INCLUDES= include .
LIBS	:=
LIBDIRS	= $(LIBNDS) $(LIBGEDS)

include $(ROOT)/common.mk

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------

LIBRARY	:= $(OUTPUT)/lib/lib$M.a
$M_LIBRARY := $(LIBRARY)

.PHONY: $(BUILD) clean install
all: $M-all
$M-all: $(BUILD) $(LIBRARY)
#---------------------------------------------------------------------------------
$(BUILD):
	mkdir -p $@

#---------------------------------------------------------------------------------
clean: $M-clean
$M-clean:
	@echo $@ ...
	rm -fr $(ppp9_BUILD) $(ppp9_LIBRARY)

$M-dox:
	-rm dox/html/*.html
	doxygen

$M-postdox: dox
	cp dox/html/doxygen.css.org dox/html/doxygen.css
	cp dox/html/tabs.css.org dox/html/tabs.css
	cd dox/html ; fe class*.html "mv % /tmp/% ; sed /tmp/% -e 's:/a>0[0-9]*:/a>:g;' > % ; echo % stripped" ; cd -
	cd dox/html ; mv classes.html /tmp/ ; sed /tmp/classes.html -e "s/>[A-Z][a-zA-Z]*::/>/g;" > classes.html ; cd -
	echo all done.

install: $M-install
$M-install:

#---------------------------------------------------------------------------------

DEPENDS	:=	$(OFILES:.o=.o.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------

$(LIBRARY): $(OFILES)
#	echo "making library using $?"
#	$(AR) -r $@ $?

checklib.elf: $(OFILES) ../main.o

../main.o: ../main.c

-include $(DEPENDS)


