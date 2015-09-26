KERN_TOOLOBJS0 = smushtool.o scattertool.o defragtool.o
DEFINE_FLAGS=-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
export CFLAGS
export OBJDIR

include src/runtime/Makefile.runtime
include src/compiler/Makefile.compiler
include src/tool/Makefile.tool

CODEOBJS :=	$(RTOBJSOUT) $(KLEERTOBJSOUT) $(RTKOBJSOUT) \
		$(TOOLOBJSOUT) $(KLEETOOLOBJSOUT) $(KTOOLOBJSOUT) \
		$(LANGOBJSOUT)

$(CODEOBJS): | $(OBJDIR)


.PHONY: code
code: compiler $(KTOOLOBJSOUT)

.PHONY: kern
kern: $(RTKOBJSOUT)
ifdef LINUX_SRCDIR
	cd kernel; make; cd ..
endif

#KLEE
.PHONY: klee
klee: $(KLEERTOBJSOUT) $(KLEETOOLOBJSOUT)

.PHONY: code-clean
code-clean: compiler-clean tools-clean
	rm -f	\
	src/compiler/lex.yy.cc \
	src/compiler/y.tab.h \
	src/compiler/parser.cc \
	src/compiler/parser.hh  \
	fsl.table.c \
	*.ll \
	$(LANGOBJSOUT) \
	$(OBJDIR)/fs/*
