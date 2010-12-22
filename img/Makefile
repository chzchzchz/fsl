FORMAT_IMGS=

all: 	fill ext2.img vfat.img nilfs2.img testfs.img \
	ext2-small.img ext2-many.img vfat-many.img  \
	iso9660-depth.img ext2-depth.img vfat-depth.img \
	vfat-postmark.img ext2-postmark.img

fill: fill.c
	gcc -O2 fill.c -o fill

clean:
	rm -f *.img fill

iso9660.img:
	rm -rf tmp_iso9660
	GEN_FILE_PATH=./tmp_iso9660/ ./gen_files.sh
	mkisofs -o iso9660.img ./tmp_iso9660
	rm -rf tmp_iso9660

iso9660-many.img:
	rm -rf tmp_iso9660_many
	mkdir tmp_iso9660_many
	GEN_FILE_PATH=./tmp_iso9660_many/ ./gen_files.sh 100
	mkisofs -o iso9660-many.img ./tmp_iso9660_many
	rm -rf tmp_iso9660_many

#unformated, 5MB:
testfs.img: fill
	./fill 1 `expr 5 \* 1024 \* 1024` $@

ext2.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 $@
	./loopback_fs_setup.sh ext2.img ext2  && ./gen_files.sh  && ./loopback_fs_teardown.sh

# 5MB
ext2-small.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=5
	yes | /sbin/mkfs.ext2 $@
	./loopback_fs_setup.sh ext2-small.img ext2  && ./gen_files.sh  && ./loopback_fs_teardown.sh

# many files
ext2-many.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 $@
	./loopback_fs_setup.sh ext2-many.img ext2  && ./gen_files.sh 100 && ./loopback_fs_teardown.sh

ext2-postmark.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 $@
	./loopback_fs_setup.sh ext2-postmark.img ext2
	./gen_postmark.sh
	./loopback_fs_teardown.sh

ext2-depth.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 $@
	./loopback_fs_setup.sh $@ ext2  && ./gen_depth.sh && ./loopback_fs_teardown.sh

vfat-depth.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat $@
	./loopback_fs_setup.sh $@ vfat  && ./gen_depth.sh && ./loopback_fs_teardown.sh

iso9660-depth.img:
	mkdir tmp_isodepth
	rm -rf tmp_isodepth/*
	GEN_FILE_PATH=./tmp_isodepth/ ./gen_depth.sh
	mkisofs -o $@  ./tmp_isodepth
	rm -rf tmp_isodepth

vfat.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat $@
	./loopback_fs_setup.sh vfat.img vfat && ./gen_files.sh  && ./loopback_fs_teardown.sh

vfat-many.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat $@
	./loopback_fs_setup.sh vfat-many.img vfat  && ./gen_files.sh 100 && ./loopback_fs_teardown.sh


# many files
vfat-postmark.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat $@
	./loopback_fs_setup.sh vfat-postmark.img vfat
	./gen_postmark.sh
	./loopback_fs_teardown.sh


nilfs2.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=256
	yes | /sbin/mkfs.nilfs2 -L mkfsnilfs2 -b 4096 $@
	./loopback_fs_setup.sh nilfs2.img nilfs2 && ./gen_files.sh  && ./loopback_fs_teardown.sh
