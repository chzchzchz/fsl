#define FUSE_USE_VERSION 25
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

static const char *fslfuse_str = "Hello World!\n";
static const char *fslfuse_path = "/hello";

static int fslfuse_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, fslfuse_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(fslfuse_str);
	}
	else
		res = -ENOENT;

	return res;
}

static int fslfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0) return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, fslfuse_path + 1, NULL, 0);

	return 0;
}

static int fslfuse_open(const char *path, struct fuse_file_info *fi)
{
	if (strcmp(path, fslfuse_path) != 0) return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY) return -EACCES;

	return 0;
}

static int fslfuse_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	if (strcmp(path, fslfuse_path) != 0) return -ENOENT;

	len = strlen(fslfuse_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, fslfuse_str + offset, size);
	} else
		size = 0;

	return size;
}

static struct fuse_operations fslfuse_oper = {
    .getattr	= fslfuse_getattr,
    .readdir	= fslfuse_readdir,
    .open	= fslfuse_open,
    .read	= fslfuse_read,
};

int tool_entry(int argc, char *argv[])
{
	char	*args[2] = {"fusebrowse", argv[0]};
	assert (argc == 1 && "NEEDS MOUNT POINT");
	return fuse_main(2, args, &fslfuse_oper);
}
