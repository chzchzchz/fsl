#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../mod/fsl_ioctl.h"

static int fd_ctl;

struct cmd_op {
	const char	*name;
	int		(*cmd_f)(int argc, char* argv[]);
};

static int cmd_get(int argc, char* argv[])
{
	int		fd_blk, err;
	const char	*blk_path;

	if (argc != 1) {
		fprintf(stderr, "Usage: fslcmd get <blkpath>");
		return -1;
	}

	blk_path = argv[0];

	/* XXX: TODO, read-only support */
	fd_blk = open(blk_path, O_RDWR);
	if (fd_blk < 0) {
		fprintf(stderr, "fslcmd: Could not open block device %s\n",
			blk_path);
		return -2;
	}
	/* tell kernel about block dev */
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_BLKDEVGET), fd_blk);
	if (err != 0) {
		fprintf(stderr, "%s: Could not get block on %s (%d)\n",
			argv[0], blk_path, err);
		return err;
	}

	close(fd_blk);

	return 0;
}

static int cmd_scatter(int argc, char* argv[])
{
	int	err;
	printf("Scattering..\n");
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_DOTOOL), FSL_DOTOOL_SCATTER);
	printf("Done Scattering. err=%d\n", err);
	return err;
}

static int cmd_defrag(int argc, char* argv[])
{
	int	err;
	printf("Defraging..\n");
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_DOTOOL), FSL_DOTOOL_DEFRAG);
	printf("Done defraging. err=%d\n", err);
	return err;
}

static int cmd_smush(int argc, char* argv[])
{
	int	err;
	printf("Smushing..\n");
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_DOTOOL), FSL_DOTOOL_SMUSH);
	printf("Done smushing. err=%d\n", err);
	return err;
}

static int cmd_vfs(int argc, char* argv[])
{
	int	err;
	printf("VFS..\n");
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_DOTOOL), FSL_DOTOOL_VFS);
	printf("VFS call done. err=%d\n", err);
	return err;
}

static int cmd_put(int argc, char* argv[])
{
	int	err;
	printf("BYEBYE BLKDEV\n");
	err = ioctl(fd_ctl, _IO(FSLIO, FSL_IOCTL_BLKDEVPUT));
	if (err != 0) {
		fprintf(stderr,
			"fslcmd: Could not drop blkdev. err=%d\n", err);
	}
	return err;
}

struct cmd_op cmds[] = {
	{.name = "get", .cmd_f = cmd_get},
	{.name = "put", .cmd_f = cmd_put},
	{.name = "scatter", .cmd_f = cmd_scatter},
	{.name = "defrag", .cmd_f = cmd_defrag},
	{.name = "smush", .cmd_f = cmd_smush},
	{.name = "vfs", .cmd_f = cmd_vfs},
	{.name = NULL}
};

int main(int argc, char* argv[])
{
	int 		err, idx;
	const char	*cmd;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s cmd [opts]\n", argv[0]);
		return -1;
	}

	cmd = argv[1];

	fd_ctl = open("/dev/fslctl", O_RDWR);
	if (fd_ctl < 0) {
		fprintf(stderr, "%s: Could not open /dev/fslctl\n", argv[0]);
		return -1;
	}

	idx = 0;
	while (cmds[idx].name != NULL) {
		if (strcmp(cmds[idx].name, cmd) == 0) {
			err = cmds[idx].cmd_f(argc-2,argv+2);
			break;
		}
		idx++;
	}
	close(fd_ctl);

	return err;
}