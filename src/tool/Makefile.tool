TOOL_NAMES= 	browser 			\
		defragtool			\
		fragtool fsck fusebrowse	\
		modify				\
		relocate			\
		scantool 			\
		scattertool smushtool		\
		thrashcopy
KLEE_TOOL_NAMES= browser scantool scattertool smushtool thrashcopy defragtool

TOOLS := $(foreach tool, $(TOOL_NAMES), $(FILESYSTEMS:%=$(tool)-%))

$(foreach fs, $(FILESYSTEMS), \
	$(eval $(fs)-objnames := fsl.$(fs).table.o fsl.$(fs).types.o))
$(foreach fs, $(FILESYSTEMS), \
	$(eval $(fs)-objs:= $($(fs)-objnames:%=$(OBJDIR)/fs/%)))
$(foreach fs, $(FILESYSTEMS), \
	$(eval $(fs)-klee-objs:= $($(fs)-objs:%.o=%.bc)))


# $(fs)-obj; object with all the $(fs) stuff linked together
$(foreach fs, $(FILESYSTEMS), $(eval $(fs)-kobj=$(OBJDIR)/fs/kfsl.$(fs).o))
$(foreach fs, $(FILESYSTEMS), $(eval $(fs)-obj=$(OBJDIR)/fs/fsl.$(fs).o))

all-fs-objs := $(foreach fs, $(FILESYSTESM), $(fs)-objs)

$(foreach tool, $(TOOL_NAMES),	\
	$(eval $(tool)-objs := $(OBJDIR)/tool/$(tool).o $(WRITE_OBJS)))

$(foreach tool, $(KLEE_TOOL_NAMES),	\
	$(eval $(tool)-klee-objs := $(OBJDIR)/klee/tool/$(tool).o $(KLEE_WRITE_OBJS)))

TOOL_BINS := $(TOOLS:%=$(BINDIR)/%)
KLEE_TOOL_BINS := $(foreach fs, $(FILESYSTEMS),	\
		$(KLEE_TOOL_NAMES:%=$(BINDIR)/klee/%-$(fs).bc))
TOOL_MMAP_BINS := $(TOOLS:%=$(BINDIR)/mmap/%)

FUSEBROWSE_OBJNAMES = fuse-browser.o fuse_node.o
fusebrowse-objs :=	$(FUSEBROWSE_OBJNAMES:%=$(OBJDIR)/tool/%) \
			$(WRITE_OBJS) \
			$(OBJDIR)/runtime/bridge.o

$(foreach fs, $(FILESYSTEMS), $(eval fsl-srctop-$(fs) := $(FSSRCDIR)/$(fs).fsl))
$(foreach fs, $(FILESYSTEMS), $(eval fsl-srcall-$(fs) := $(wildcard $(FSSRCDIR)/$(fs)*.fsl)))

TOOL_CFLAGS = $(CFLAGS) -Isrc/

KERN_TOOLOBJS=$(KERN_TOOLOBJS0:%=tool/%)
TOOLOBJS=$(patsubst %.c,%.o,$(wildcard tool/*.c))

TOOLOBJSOUT := $(TOOLOBJS:%=$(OBJDIR)/%)
KLEETOOLOBJSOUT := $(TOOLOBJS:%=$(OBJDIR)/klee/%)
KTOOLOBJSOUT := $(KERN_TOOLOBJS:%=$(OBJDIR)/kernel/%)

####### TOOL PROGRAMS ###############
$(foreach tool, $(TOOLS),						\
	$(eval $(tool)-deps := 						\
		$($(lastword $(subst -,$(space),$(tool)))-obj)		\
		$($(firstword $(subst -,$(space),$(tool)))-objs) 	\
		$(RT_PREAD_OBJS)))

$(foreach tool, $(TOOLS),						\
	$(eval $(tool)-klee-deps := 					\
		$($(lastword $(subst -,$(space),$(tool)))-klee-objs)	\
		$($(firstword $(subst -,$(space),$(tool)))-klee-objs) 	\
		$(KLEE_RT_PREAD_OBJS)))

$(foreach tool, $(TOOLS),						\
	$(eval $(tool)-mmap-deps := 					\
		$($(lastword $(subst -,$(space),$(tool)))-obj)		\
		$($(firstword $(subst -,$(space),$(tool)))-objs) 	\
		$(RT_MMAP_OBJS)))

$(foreach fs, $(FILESYSTEMS), $(eval fusebrowse-$fs-deps += $(fusebrowse-obj)))
$(foreach fs, $(FILESYSTEMS), $(eval fusebrowse-$fs-mmap-deps += $(fusebrowse-obj)))

.SECONDEXPANSION:
$(TOOL_BINS) : $(BINDIR)/% : $$($$*-deps)
	$(CC) $(TOOL_CFLAGS) -lfuse -o $@ $^

.SECONDEXPANSION:
$(TOOL_MMAP_BINS) : $(BINDIR)/mmap/% : $$($$*-mmap-deps)
	$(CC) $(TOOL_CFLAGS) -lfuse -o $@ $^

.SECONDEXPANSION:
$(KLEE_TOOL_BINS) : $(BINDIR)/klee/%.bc : $$($$*-klee-deps)
	llvm-link -o $@ $^

######################################

### TOOL OBJECTS ######
$(OBJDIR)/klee/tool/%.o: src/tool/%.c
	$(LLVMCC) -DUSE_KLEE $(RTCFLAGS) -emit-llvm -Isrc/runtime/ -c $< -o $@

$(OBJDIR)/tool/%.o: src/tool/%.c
	$(CC) $(RTCFLAGS) -Isrc/runtime/ -c $< -o $@

$(OBJDIR)/kernel/tool/%.o: src/tool/%.c
	$(CC) $(RTKCFLAGS) -Isrc/runtime/ -c $< -o $@
###################

#############################
#########FS OBJECTS#################
####################################
# KERNEL SPACE FS MODULES
$(OBJDIR)/fs/kfsl.%.o:	$(OBJDIR)/fs/kfsl.%.table.o \
			$(OBJDIR)/fs/kfsl.%.types.o
	ld -r -o $@ $^

$(OBJDIR)/fs/kfsl.%.table.o: $(OBJDIR)/fs/fsl.%.table.c
	$(CC) -mcmodel=kernel -mno-red-zone $(TOOL_CFLAGS) -I.. -c $^ -o $@

$(OBJDIR)/fs/kfsl.%.types.o: $(OBJDIR)/fs/kfsl.%.types.s
	as -g $< -o $@

# XXX -disable-red-zone should already be disabled in .bc file; the flag
# has been deprecated. Possibly need type annotations.
$(OBJDIR)/fs/kfsl.%.types.s: $(OBJDIR)/fs/fsl.%.types.bc
	llc -code-model=kernel $(LLC_FLAGS) -o $@ $<

# USER SPACE FS MODULES
$(OBJDIR)/fs/fsl.%.o:	$(OBJDIR)/fs/fsl.%.table.o \
			$(OBJDIR)/fs/fsl.%.types.o
	ld -r -o $@ $^

$(OBJDIR)/fs/fsl.%.table.o: $(OBJDIR)/fs/fsl.%.table.c
	$(CC) $(TOOL_CFLAGS) -I.. -c -o $@ $<

$(OBJDIR)/fs/fsl.%.table.bc: $(OBJDIR)/fs/fsl.%.table.c
	$(LLVMCC) -emit-llvm $(TOOL_CFLAGS) -I.. -c -o $@ $<

$(OBJDIR)/fs/fsl.%.types.o: $(OBJDIR)/fs/fsl.%.types.s
	as -g -o $@ $<

$(OBJDIR)/fs/fsl.%.types.s: $(OBJDIR)/fs/fsl.%.types.bc
	llc $(LLC_FLAGS) -o $@ $<

$(OBJDIR)/fs/fsl.%.types.bc: $(OBJDIR)/fs/fsl.%.types.ll
	opt $(OPT_FLAGS) -o $@ $<

$(OBJDIR)/fs/fsl.%.table.c $(OBJDIR)/fs/fsl.%.types.ll: \
		$(fsl-srcall-%)				\
		$(BINDIR)/lang
	cd $(OBJDIR)/fs/ && $(BINDIR)/lang $(*) $(FSSRCDIR)/$(*).fsl 2>$(*).err

#################

.PHONY: tools
tools: $(TOOL_BINS) $(TOOL_MMAP_BINS) tools-klee

.PHONY: tools-clean
tools-clean:
	rm -f $(TOOL_BINS) $(TOOL_MMAP_BINS) $(all-fs-objs) $(KLEE_TOOL_BINS) $(OBJDIR)/fs/kfsl.$(FSNAME).o $(TOOLOBJSOUT) $(KLEETOOLOBJSOUT) $(KTOOLOBJSOUT)

.PHONY: tools-klee
tools-klee: $(KLEE_TOOL_BINS)
