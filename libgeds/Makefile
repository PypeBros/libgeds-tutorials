#---------------------------------------------------------------------------------
.SUFFIXES:

ROOT ?= $(abspath $(CURDIR)/..)
M 	:= libgeds
CPU := arm9
TARGET 	:= $M
DATA	:= data  
INCLUDES= include
LIBS	:= ../main.o -lppp9 -lfat -lnds9 -ldswifi9
LIBDIRS	= $(LIBNDS) $(LIBPPP9)

include $(ROOT)/common.mk

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------

LIBRARY	:= $(OUTPUT)/lib/$M.a
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
	rm -fr $(libgeds_BUILD) $(libgeds_LIBRARY)

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

XSLT_FLAGS = --nonet --xinclude
XSLT_PARAM = --param toc.section.depth 4 --param generate.consistent.ids 1

stamp-xml-doxygen: ${doxygen_outdir}/xml
	-(srcdir=`cd ${top_srcdir}; ${PWD_COMMAND}`; \
	  builddir=`cd ..; ${PWD_COMMAND}`; \
	  ${SHELL} ${doxygen_script} \
	  --host_alias=${host_alias} --mode=xml $${srcdir} $${builddir} NO)
	$(STAMP) stamp-xml-doxygen

stamp-xml-single-doxygen: stamp-xml-doxygen
	@echo "Generating doxygen xml single file..."
	$(XSLTPROC) ${doxygen_outdir}/xml/combine.xslt \
	${doxygen_outdir}/xml/index.xml > ${api_xml};
	$(STAMP) stamp-xml-single-doxygen


stamp-epub-docbook: stamp-xml-single-docbook ${docbook_outdir}/epub
	@echo "Generating epub files..."
	if [ ! -d "${docbook_outdir}/images" ]; then \
	  $(LN_S) ${top_srcdir}/doc/xml/images ${docbook_outdir}/; \
	fi
	$(XSLTPROC) $(XSLT_PARAM) $(XSLT_FLAGS) \
	--stringparam base.dir "${docbook_outdir}/epub/OEBPS/" \
	$(XSL_EPUB_STYLE) ${top_srcdir}/doc/xml/spine.xml
	$(INSTALL_DATA)	$(xml_images) ${docbook_outdir}/epub/OEBPS/images
	zip -X0 ${manual_epub} ${docbook_outdir}/epub/mimetype
	zip -r -X9 ${manual_epub} ${docbook_outdir}/epub/META-INF ${docbook_outdir}/epub/OEBPS
	$(STAMP) stamp-epub-docbook

stamp-xml-single-docbook: $(xml_sources) ${docbook_outdir}/xml
	@echo "Generating XML single..."
	$(XMLLINT) $(XMLLINT_FLAGS) \
	${top_srcdir}/doc/xml/manual/spine.xml > ${manual_xml};
	$(XMLLINT) $(XMLLINT_FLAGS) \
	${top_srcdir}/doc/xml/spine.xml > ${set_xml};
	$(STAMP) stamp-xml-single-docbook

xml_sources_basic = \
	${xml_dir}/spine.xml \
	${xml_dir}/authors.xml \
	${xml_dir}/api.xml \
	${xml_dir}/faq.xml

xml_sources_manual = 
xml_sources_extra = 
xml_sources = \
	${xml_sources_basic} \
	${xml_sources_manual} \
	${xml_sources_extra}
