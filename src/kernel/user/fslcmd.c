#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../mod/fsl_ioctl.h"

int main(int argc, char* argv[])
{
	int 	fd_ctl, fd_blk, err;
	char	*blk_path;

	blk_path = argv[1];

	fd_ctl = open("/dev/fslctl", O_RDWR);
	if (fd_ctl < 0) {
		fprintf(stderr, "%s: Could not open /dev/fslctl\n", argv[0]);
		return -1;
	}

	/* XXX: TODO, read-only */
	fd_blk = open(blk_path, O_RDWR);
	if (fd_blk < 0) {
		fprintf(stderr, "%s: Could not open block device %s\n",
			argv[0], blk_path);
		return -2;
	}
	/* tell kernel about block dev */
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_BLKDEVGET), fd_blk);
	if (err != 0) {
		fprintf(stderr, "%s: Could not get block on %s (%d)\n",
			argv[0], blk_path, err);
		return err;
	}

	/* ready for commands !*/
	printf("Scattering..\n");
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_DOTOOL), FSL_DOTOOL_SCATTER);
	printf("Done Scattering. err=%d\n", err);
	if (err != 0) return err;

	printf("Defraging..\n");
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_DOTOOL), FSL_DOTOOL_DEFRAG);
	printf("Done defraging. err=%d\n", err);
	if (err != 0) return err;

	printf("Smushing..\n");
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_DOTOOL), FSL_DOTOOL_SMUSH);
	printf("Done smushing. err=%d\n", err);
	if (err != 0) return err;


	printf("BYEBYE BLKDEV\n");
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_BLKDEVPUT), fd_blk);
	if (err != 0) {
		fprintf(stderr, "%s: Could not drop blkdev %s. err=%d\n",
			argv[0], blk_path, err);
		return err;
	}

	close(fd_ctl);
	close(fd_blk);

	return 0;
}