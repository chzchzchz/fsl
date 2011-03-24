FILESYSTEMS := ext2 vfat nilfs2 iso9660 testfs reiserfs xfs minix btrfs
TOOL_NAMES=browser scantool modify fragtool relocate defragtool scattertool smushtool fsck
TOOLS_BROWSER := $(FILESYSTEMS:%=browser-%)
TOOLS_SCAN := $(FILESYSTEMS:%=scantool-%)
TOOLS_MODIFY := $(FILESYSTEMS:%=modify-%)
TOOLS_FRAG := $(FILESYSTEMS:%=fragtool-%)
TOOLS_RELOC := $(FILESYSTEMS:%=relocate-%)
TOOLS_DEFRAG := $(FILESYSTEMS:%=defragtool-%)
TOOLS_SCATTER := $(FILESYSTEMS:%=scattertool-%)
TOOLS_SMUSH := $(FILESYSTEMS:%=smushtool-%)

TOOLS_ALL=$(TOOLS_BROWSER) $(TOOLS_SCAN) $(TOOLS_MODIFY) $(TOOLS_FRAG) $(TOOLS_RELOC) $(TOOLS_DEFRAG) $(TOOLS_SCATTER) $(TOOLS_SMUSH)

export CFLAGS

all: $(TOOLS_BROWSER)

%: browser-%
	echo $@

browser-testfs: $(wildcard ../../fs/testfs*.fsl)
	FSNAME=testfs make -f Makefile.tools
browser-iso9660: $(wildcard ../../fs/iso9660*.fsl)
	FSNAME=iso9660 make -f Makefile.tools
browser-nilfs2: $(wildcard ../../fs/nilfs2*.fsl)
	FSNAME=nilfs2 make -f Makefile.tools
browser-vfat: $(wildcard ../../fs/vfat*.fsl)
	FSNAME=vfat make -f Makefile.tools
browser-ext2: $(wildcard ../../fs/ext2*fsl)
	FSNAME=ext2 make -f Makefile.tools
browser-reiserfs: $(wildcard ../../fs/reiserfs*fsl)
	FSNAME=reiserfs make -f Makefile.tools
browser-xfs: $(wildcard ../../fs/xfs*fsl)
	FSNAME=xfs make -f Makefile.tools
browser-minix: $(wildcard ../../fs/minix*fsl)
	FSNAME=minix make -f Makefile.tools
browser-btrfs: $(wildcard ../../fs/btrfs*fsl)
	FSNAME=btrfs make -f Makefile.tools


CLEANTARGETS=$(FILESYSTEMS:%=clean-%)

clean: $(CLEANTARGETS)

clean-%:
	FSNAME=`echo $@ | cut -f2 -d'-'`  make -f Makefile.tools clean
