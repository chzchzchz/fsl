FILESYSTEMS := ext2 vfat nilfs2 iso9660 testfs
TOOL_NAMES=browser scantool modify fragtool
TOOLS_BROWSER := $(FILESYSTEMS:%=browser-%)
TOOLS_SCAN := $(FILESYSTEMS:%=scantool-%)
TOOLS_MODIFY := $(FILESYSTEMS:%=modify-%)
TOOLS_FRAG := $(FILESYSTEMS:%=fragtool-%)

TOOLS_ALL=$(TOOLS_BROWSER) $(TOOLS_SCAN) $(TOOLS_MODIFY) $(TOOLS_FRAG)

export CFLAGS

all: $(TOOLS_ALL)

fragtool-% scantool-% modify-% browser-%: ../../fs/%.fsl
	FSNAME=$* make -f Makefile.tools

clean:
	rm  -f *.s *.o *.bc *.ll fsl.*.*.c $(TOOLS_ALL)
