all: ext2.img vfat.img nilfs.img

format: ext2.img vfat.img nilfs.img
	./loopback_fs_setup.sh ext2.img ext2  && ./gen_files.sh  && ./loopback_fs_teardown.sh
	./loopback_fs_setup.sh vfat.img vfat && ./gen_files.sh  && ./loopback_fs_teardown.sh
	./loopback_fs_setup.sh nilfs.img nilfs2 && ./gen_files.sh  && ./loopback_fs_teardown.sh

clean:
	rm -f *.img


ext2.img:
	dd if=/dev/zero of=ext2.img bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 ext2.img

vfat.img:
	dd if=/dev/zero of=vfat.img bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat vfat.img

nilfs.img:
	dd if=/dev/zero of=nilfs.img bs=`expr 1024 \* 1024` count=256
	yes | /sbin/mkfs.nilfs2 -L mkfsnilfs -b 4096 nilfs.img
