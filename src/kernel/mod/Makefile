EXTRA_CFLAGS := -I$(src)/../../runtime/
KOBJ_DIR := ../../../obj/kernel
KTOOLOBJ_DIR := ../../../obj/kernel/tool
FSKOBJ_DIR := ../../../obj/fs

obj-m := fsl.o
fsl-objs := fsl_entry.o debug.o alloc.o io.o cache.o info_kern.o	\
	$(KOBJ_DIR)/bitmap.o		\
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
	$(FSKOBJ_DIR)/kfsl.ext2.o	\
	$(KTOOLOBJ_DIR)/smushtool.o	\
	$(KTOOLOBJ_DIR)/defragtool.o	\
	$(KTOOLOBJ_DIR)/scattertool.o

clean:
	rm -f *.o *.ko fsl.mod.c
