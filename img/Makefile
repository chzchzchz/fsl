all: ext2.img vfat.img nilfs2.img ext2-small.img ext2-many.img testfs.img vfat-many.img ext2-postmark.img vfat-postmark.img

format: ext2.img vfat.img nilfs2.img ext2-small.img ext2-many.img format-ext2-many.img  format-vfat-many.img
	./loopback_fs_setup.sh ext2.img ext2  && ./gen_files.sh  && ./loopback_fs_teardown.sh
	./loopback_fs_setup.sh ext2-small.img ext2  && ./gen_files.sh  && ./loopback_fs_teardown.sh
	./loopback_fs_setup.sh vfat.img vfat && ./gen_files.sh  && ./loopback_fs_teardown.sh
	./loopback_fs_setup.sh nilfs2.img nilfs2 && ./gen_files.sh  && ./loopback_fs_teardown.sh

clean:
	rm -f *.img


format-ext2-postmark.img: ext2-postmark.img
	./loopback_fs_setup.sh ext2-postmark.img ext2  && ./gen_postmark.sh && ./loopback_fs_teardown.sh

format-ext2-many.img: ext2-many.img
	./loopback_fs_setup.sh ext2-many.img ext2  && ./gen_files.sh 100 && ./loopback_fs_teardown.sh

format-vfat-many.img: vfat-many.img
	./loopback_fs_setup.sh vfat-many.img vfat  && ./gen_files.sh 100 && ./loopback_fs_teardown.sh

format-vfat-postmark.img: vfat-postmark.img
	./loopback_fs_setup.sh vfat-postmark.img vfat  && ./gen_postmark.sh && ./loopback_fs_teardown.sh



#unformated, 5MB:
testfs.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=5

ext2.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 $@

# 5MB
ext2-small.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=5
	yes | /sbin/mkfs.ext2 $@

# many files
ext2-many.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 $@

ext2-postmark.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 $@

vfat.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat $@

vfat-many.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat $@

# many files
vfat-postmark.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat $@



nilfs2.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=256
	yes | /sbin/mkfs.nilfs2 -L mkfsnilfs2 -b 4096 $@
