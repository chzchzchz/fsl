FILESYSTEMS := ext2 vfat nilfs2 iso9660 testfs
TOOL_NAMES=browser scantool modify
TOOLS_BROWSER := $(FILESYSTEMS:%=browser-%)
TOOLS_SCAN := $(FILESYSTEMS:%=scantool-%)
TOOLS_MODIFY := $(FILESYSTEMS:%=modify-%)

TOOLS_ALL=$(TOOLS_BROWSER) $(TOOLS_SCAN) $(TOOLS_MODIFY)

all: $(TOOLS_ALL)

scantool-% modify-% browser-%: ../../fs/%.fsl
	FSNAME=$* make -f Makefile.tools

clean:
	rm  -f *.s *.o *.bc *.ll fsl.*.*.c scantool* browser*
