EXT2_TOOLS= scantool-ext2 browser-ext2
VFAT_TOOLS= scantool-vfat browser-vfat
NILFS_TOOLS= scantool-nilfs2 browser-nilfs2
ISO9660_TOOLS= scantool-iso9660 browser-iso9660
TEST_TOOLS= browser-testfs scantool-testfs

all: $(EXT2_TOOLS) $(VFAT_TOOLS) $(NILFS_TOOLS) $(TEST_TOOLS) $(ISO9660_TOOLS)

clean:
	rm  -f *.s *.o *.bc *.ll fsl.*.*.c scantool* browser*

browser-iso9660: ../../fs/iso9660.fsl
	FSNAME=iso9660 make -f Makefile.tools
scantool-iso9660: browser-iso9660

browser-ext2: ../../fs/ext2.fsl
	FSNAME=ext2 make -f Makefile.tools
scantool-ext2: browser-vfat

browser-vfat: ../../fs/vfat.fsl
	FSNAME=vfat make -f Makefile.tools
scantool-vfat: browser-vfat

browser-nilfs2: ../../fs/nilfs2.fsl
	FSNAME=nilfs2 make -f Makefile.tools
scantool-nilfs2: browser-nilfs2


browser-testfs: ../../fs/testfs.fsl
	FSNAME=testfs make -f Makefile.tools
scantool-testfs: browser-testfs

