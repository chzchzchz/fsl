EXTRA_CFLAGS := -I$(src)/../../runtime/
KOBJ_DIR := ../../../obj/kernel
KTOOLOBJ_DIR := ../../../obj/kernel/tool
FSKOBJ_DIR := ../../../obj/fs

BASEOBJS= fsl_entry.o debug.o alloc.o io.o cache.o info_kern.o rt.o \
	inode.o super.o	\
	$(KOBJ_DIR)/bitmap.o		\
	$(KOBJ_DIR)/bridge.o		\
	$(KOBJ_DIR)/choice.o		\
	$(KOBJ_DIR)/io.o		\
	$(KOBJ_DIR)/log.o		\
	$(KOBJ_DIR)/lookup.o		\
	$(KOBJ_DIR)/max.o		\
	$(KOBJ_DIR)/runtime.o		\
	$(KOBJ_DIR)/writepkt.o		\
	$(KOBJ_DIR)/scan.o		\
	$(KOBJ_DIR)/type_info.o		\
	$(KOBJ_DIR)/type_print.o	\
	$(KOBJ_DIR)/virt.o		\
	$(KTOOLOBJ_DIR)/smushtool.o	\
	$(KTOOLOBJ_DIR)/defragtool.o	\
	$(KTOOLOBJ_DIR)/scattertool.o	\

obj-m := fslext2.o fslvfat.o fsliso9660.o  fslreiserfs.o fslxfs.o fslminix.o
fslext2-objs := $(BASEOBJS) $(FSKOBJ_DIR)/kfsl.ext2.o
fslvfat-objs := $(BASEOBJS) $(FSKOBJ_DIR)/kfsl.vfat.o
fsliso9660-objs := $(BASEOBJS) $(FSKOBJ_DIR)/kfsl.iso9660.o
fslreiserfs-objs := $(BASEOBJS) $(FSKOBJ_DIR)/kfsl.reiserfs.o
fslxfs-objs := $(BASEOBJS) $(FSKOBJ_DIR)/kfsl.xfs.o
fslminix-objs := $(BASEOBJS) $(FSKOBJ_DIR)/kfsl.minix.o


clean:
	rm -f *.o *.ko fsl.mod.c
