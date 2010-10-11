EXT2_TOOLS= scantool-ext2 browser-ext2
VFAT_TOOLS= scantool-vfat browser-vfat
NILFS_TOOLS= scantool-nilfs2 browser-nilfs2
TEST_TOOLS= browser-testfs scantool-testfs

all: $(EXT2_TOOLS) $(VFAT_TOOLS) $(NILFS_TOOLS) $(TEST_TOOLS)

clean:
	rm  -f *.s *.o *.bc *.ll fsl.*.*.c scantool* browser*

browser-ext2:
	FSNAME=ext2 make -f Makefile.tools
scantool-ext2: browser-vfat

browser-vfat:
	FSNAME=vfat make -f Makefile.tools
scantool-vfat: browser-vfat

browser-nilfs2:
	FSNAME=nilfs2 make -f Makefile.tools
scantool-nilfs2: browser-nilfs2


browser-testfs:
	FSNAME=testfs make -f Makefile.tools
scantool-testfs: browser-testfs

