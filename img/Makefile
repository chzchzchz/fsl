all: ext2.img vfat.img nilfs2.img ext2-small.img testfs.img

format: ext2.img vfat.img nilfs2.img ext2-small.img
	./loopback_fs_setup.sh ext2.img ext2  && ./gen_files.sh  && ./loopback_fs_teardown.sh
	./loopback_fs_setup.sh ext2-small.img ext2  && ./gen_files.sh  && ./loopback_fs_teardown.sh
	./loopback_fs_setup.sh vfat.img vfat && ./gen_files.sh  && ./loopback_fs_teardown.sh
	./loopback_fs_setup.sh nilfs2.img nilfs2 && ./gen_files.sh  && ./loopback_fs_teardown.sh

clean:
	rm -f *.img

#unformated, 5MB:
testfs.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=5

# 5MB
ext2-small.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=5
	yes | /sbin/mkfs.ext2 $@

ext2.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 $@

vfat.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat $@

nilfs2.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=256
	yes | /sbin/mkfs.nilfs2 -L mkfsnilfs2 -b 4096 $@
