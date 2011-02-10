EXTRA_CFLAGS := -I$(src)/../runtime/
obj-m := fsl.o
fsl-objs := fsl_entry.o debug.o alloc.o io.o	\
	../../obj/runtime/bitmap.o		\
	../../obj/runtime/choice.o		\
	../../obj/runtime/io.o			\
	../../obj/runtime/log.o			\
	../../obj/runtime/lookup.o		\
	../../obj/runtime/max.o			\
	../../obj/runtime/runtime.o		\
	../../obj/runtime/writepkt.o		\
	../../obj/runtime/type_info.o		\
	../../obj/runtime/virt.o
clean:
	rm -f *.o *.ko
