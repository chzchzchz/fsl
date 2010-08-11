EXT2_OBJS= fsl.ext2.table.o fsl.ext2.types.o
EXT2_TOOLS= scantool-ext2 browser-ext2
VFAT_OBJS= fsl.vfat.table.o fsl.vfat.types.o
VFAT_TOOLS= scantool-vfat browser-vfat
SCANTOOL_OBJS=../browser.o ../runtime.o
BROWSER_OBJS=../browser.o ../scantool.o

all: $(EXT2_TOOLS) $(VFAT_TOOLS)

clean:
	rm  -f *.s *.o *.bc *.ll fsl.*.*.c scantool* browser*

browser-ext2: $(EXT2_OBJS)
	gcc -O3 -o $@ $^ ../browser.o ../runtime.o 

scantool-ext2: $(EXT2_OBJS)
	gcc -O3 -o $@ $^ ../scantool.o ../runtime.o 

browser-vfat: $(VFAT_OBJS)
	gcc -O3 -o $@ $^ ../browser.o ../runtime.o 

scantool-vfat: $(VFAT_OBJS)
	gcc -O3 -o $@ $^ ../scantool.o ../runtime.o 


fsl.ext2.types.o: fsl.ext2.types.s
	as $< -o $@

fsl.ext2.types.s: fsl.ext2.types.bc
	llc -O3 $< -o $@

fsl.ext2.types.bc: fsl.ext2.types.ll
	opt -O3 $^ >$@

fsl.ext2.types.ll:
	../lang ext2 <../../fs/ext2.fsl

fsl.ext2.table.c:
	../lang ext2 <../../fs/ext2.fsl

fsl.ext2.table.o: fsl.ext2.table.c
	gcc -O3 -c $^ -o $@

fsl.vfat.types.o: fsl.vfat.types.s
	as $< -o $@

fsl.vfat.types.s: fsl.vfat.types.bc
	llc -O3 $< -o $@

fsl.vfat.types.bc: fsl.vfat.types.ll
	opt -O3 $^ >$@

fsl.vfat.types.ll:
	../lang vfat <../../fs/vfat.fsl

fsl.vfat.table.c:
	../lang vfat <../../fs/vfat.fsl

fsl.vfat.table.o: fsl.vfat.table.c
	gcc -O3 -c $^ -o $@

