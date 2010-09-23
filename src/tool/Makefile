EXT2_OBJS= fsl.ext2.table.o fsl.ext2.types.o
EXT2_TOOLS= scantool-ext2 browser-ext2
VFAT_OBJS= fsl.vfat.table.o fsl.vfat.types.o
VFAT_TOOLS= scantool-vfat browser-vfat
NILFS_OBJS= fsl.nilfs.table.o fsl.nilfs.types.o
NILFS_TOOLS= scantool-nilfs browser-nilfs
RT_OBJS=../runtime/runtime.o 	\
	../runtime/type_info.o	\
	../runtime/virt.o	\
	../runtime/dyn.o	\
	../runtime/io.o		\
	../runtime/max.o

SCANTOOL_OBJS=../runtime/scantool.o  $(RT_OBJS)
BROWSER_OBJS=../runtime/browser.o $(RT_OBJS)
MODIFY_OBJS=../runtime/modify.o $(RT_OBJS)
CFLAGS=-O3 -g

all: $(EXT2_TOOLS) $(VFAT_TOOLS) $(NILFS_TOOLS)

clean:
	rm  -f *.s *.o *.bc *.ll fsl.*.*.c scantool* browser*

browser-ext2: $(EXT2_OBJS)
	gcc $(CFLAGS) -o $@ $^  $(BROWSER_OBJS)

scantool-ext2: $(EXT2_OBJS)
	gcc $(CFLAGS) -o $@ $^  $(SCANTOOL_OBJS)

browser-vfat: $(VFAT_OBJS)
	gcc $(CFLAGS) -o $@ $^ $(BROWSER_OBJS)

scantool-vfat: $(VFAT_OBJS)
	gcc $(CFLAGS) -o $@ $^ $(SCANTOOL_OBJS)

browser-nilfs: $(NILFS_OBJS)
	gcc $(CFLAGS) -o $@ $^ $(BROWSER_OBJS)

scantool-nilfs: $(NILFS_OBJS)
	gcc $(CFLAGS) -o $@ $^ $(SCANTOOL_OBJS)



fsl.ext2.types.o: fsl.ext2.types.s
	as $< -o $@

fsl.ext2.types.s: fsl.ext2.types.bc
	llc -O3 $< -o $@

fsl.ext2.types.bc: fsl.ext2.types.ll
	opt -O3 $^ >$@

fsl.ext2.types.ll: ../../fs/ext2.fsl
	../lang ext2 <$^

fsl.ext2.table.c: ../../fs/ext2.fsl
	../lang ext2 <$^

fsl.ext2.table.o: fsl.ext2.table.c
	gcc $(CFLAGS) -I.. -c $^ -o $@



fsl.vfat.types.o: fsl.vfat.types.s
	as $< -o $@

fsl.vfat.types.s: fsl.vfat.types.bc
	llc -O3  $< -o $@

fsl.vfat.types.bc: fsl.vfat.types.ll
	opt -O3 $^ >$@

fsl.vfat.types.ll: ../../fs/vfat.fsl
	../lang vfat <$^

fsl.vfat.table.c: ../../fs/vfat.fsl
	../lang vfat <$^

fsl.vfat.table.o: fsl.vfat.table.c
	gcc $(CFLAGS) -I.. -c $^ -o $@


fsl.nilfs.types.o: fsl.nilfs.types.s
	as $< -o $@

fsl.nilfs.types.s: fsl.nilfs.types.bc
	llc -O3 $< -o $@

fsl.nilfs.types.bc: fsl.nilfs.types.ll
	opt -O3 $^ >$@

fsl.nilfs.types.ll: ../../fs/nilfs2.fsl
	../lang nilfs <$^

fsl.nilfs.table.c: ../../fs/nilfs2.fsl
	../lang nilfs <$^

fsl.nilfs.table.o: fsl.nilfs.table.c
	gcc $(CFLAGS) -I.. -c $^ -o $@

