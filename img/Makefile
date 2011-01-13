include Makefile.depth

all: 	fill ext2.img vfat.img nilfs2.img testfs.img reiserfs.img \
	ext2-small.img ext2-many.img vfat-many.img  reiserfs-many.img iso9660-many.img vfat-32-many.img \
	reiserfs-many-blockfile.img			\
	iso9660-depth.img ext2-depth.img vfat-depth.img reiserfs-depth.img \
	vfat-postmark.img ext2-postmark.img reiserfs-postmark.img \
	ext2-linux-4gb.img reiserfs-linux-4gb.img vfat-linux-4gb.img iso9660-linux-4gb.img

depth: $(DEPTH_IMGS)

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

iso9660-postmark.img:
	rm -rf tmp_iso9660_postmark
	mkdir tmp_iso9660_postmark
	GEN_FILE_PATH=./tmp_iso9660_postmark/ ./gen_postmark.sh 100
	mkisofs -o iso9660-postmark.img ./tmp_iso9660_postmark
	rm -rf tmp_iso9660_postmark

iso9660-linux-4gb.img:
	rm -rf isolinuxtmp
	mkdir isolinuxtmp
	mkdir isolinuxtmp/{1,2,3,4}
	tar jxvf  /usr/src/linux-2.6.36.2.tar.bz2 -C isolinuxtmp/1
	tar jxvf  /usr/src/linux-2.6.36.2.tar.bz2 -C isolinuxtmp/2
	tar jxvf  /usr/src/linux-2.6.36.2.tar.bz2 -C isolinuxtmp/3
	tar jxvf  /usr/src/linux-2.6.36.2.tar.bz2 -C isolinuxtmp/4
	mkisofs -o iso9660-linux-4gb.img ./isolinuxtmp
	rm -rf isolinuxtmp

#unformated, 5MB:
testfs.img: fill
	./fill 1 `expr 5 \* 1024 \* 1024` $@

ext2-linux-4gb.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=4096
	yes | /sbin/mkfs.ext2 $@
	./loopback_fs_setup.sh $@ ext2
	mkdir /mnt/FSTEST/{1,2,3,4}
	tar jxvf /usr/src/linux-2.6.36.2.tar.bz2 -C /mnt/FSTEST/1
	tar jxvf /usr/src/linux-2.6.36.2.tar.bz2 -C /mnt/FSTEST/2
	tar jxvf /usr/src/linux-2.6.36.2.tar.bz2 -C /mnt/FSTEST/3
	tar jxvf /usr/src/linux-2.6.36.2.tar.bz2 -C /mnt/FSTEST/4
	./loopback_fs_teardown.sh

reiserfs-linux-4gb.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=4096
	yes | /sbin/mkfs.reiserfs -f $@
	./loopback_fs_setup.sh $@ reiserfs
	mkdir /mnt/FSTEST/{1,2,3,4}
	tar jxvf /usr/src/linux-2.6.36.2.tar.bz2 -C /mnt/FSTEST/1
	tar jxvf /usr/src/linux-2.6.36.2.tar.bz2 -C /mnt/FSTEST/2
	tar jxvf /usr/src/linux-2.6.36.2.tar.bz2 -C /mnt/FSTEST/3
	tar jxvf /usr/src/linux-2.6.36.2.tar.bz2 -C /mnt/FSTEST/4
	./loopback_fs_teardown.sh

vfat-linux-4gb.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=4096
	yes | /usr/sbin/mkfs.msdos -f 1 $@
	./loopback_fs_setup.sh $@ vfat
	rm -rf linuxtmp
	mkdir linuxtmp
	tar jxvf  /usr/src/linux-2.6.36.2.tar.bz2 -C linuxtmp
	mkdir /mnt/FSTEST/{1,2,3,4}
	cp -f -r -L -u linuxtmp/linux-2.6.36.2 /mnt/FSTEST/1/; true
	cp -f -r -L -u linuxtmp/linux-2.6.36.2 /mnt/FSTEST/2/; true
	cp -f -r -L -u linuxtmp/linux-2.6.36.2 /mnt/FSTEST/3/; true
	cp -f -r -L -u linuxtmp/linux-2.6.36.2 /mnt/FSTEST/4/; true
	./loopback_fs_teardown.sh
	rm -rf linuxtmp


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
	./loopback_fs_setup.sh ext2-many.img ext2
	./gen_files.sh 100
	./loopback_fs_teardown.sh

ext2-postmark.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.ext2 $@
	./loopback_fs_setup.sh ext2-postmark.img ext2
	./gen_postmark.sh
	mkdir /mnt/FSTEST/lost+found/
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
	./loopback_fs_setup.sh $@ vfat  && ./gen_files.sh 100 && ./loopback_fs_teardown.sh

vfat-32-many.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat -F 32 $@
	./loopback_fs_setup.sh $@ vfat  && ./gen_files.sh 100 && ./loopback_fs_teardown.sh


vfat-manymore.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /usr/sbin/mkfs.vfat $@
	./loopback_fs_setup.sh $@ vfat  && ./gen_files.sh 120 && ./loopback_fs_teardown.sh

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

reiserfs.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.reiserfs -s 1024 -f $@
	./loopback_fs_setup.sh $@ reiserfs
	./gen_files.sh
	./loopback_fs_teardown.sh


reiserfs-many.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.reiserfs -s 1024 -f $@
	./loopback_fs_setup.sh $@ reiserfs
	./gen_files.sh 100
	./loopback_fs_teardown.sh

reiserfs-postmark.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.reiserfs -s 1024 -f $@
	./loopback_fs_setup.sh $@ reiserfs
	./gen_postmark.sh
	./loopback_fs_teardown.sh

reiserfs-depth.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.reiserfs -s 1024 -f $@
	./loopback_fs_setup.sh $@ reiserfs
	./gen_depth.sh
	./loopback_fs_teardown.sh

reiserfs-many-blockfile.img:
	dd if=/dev/zero of=$@ bs=`expr 1024 \* 1024` count=100
	yes | /sbin/mkfs.reiserfs -s 1024 -f $@
	./loopback_fs_setup.sh $@ reiserfs
	GEN_FILE_LENGTH=4096 ./gen_files.sh 75
	./loopback_fs_teardown.sh
