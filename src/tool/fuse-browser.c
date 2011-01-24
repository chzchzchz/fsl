#define FUSE_USE_VERSION 25
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include "runtime.h"
#include "type_info.h"
#include "fuse_node.h"

FILE			*out_file;
static const char	*fslfuse_str = "Hello World!\n";
static uid_t		our_uid;
static gid_t		our_gid;
time_t			open_time;

static int fslfuse_getattr_type(struct type_info* ti, struct stat* stbuf)
{
	stbuf->st_mode = S_IFDIR | 0500;
	stbuf->st_nlink = 2;
	stbuf->st_size = ti_size(ti) / 8;
	stbuf->st_blksize = 512;
	return 0;
}

static int fslfuse_getattr_array(struct fsl_fuse_node* fn, struct stat* stbuf)
{
	const struct fsl_rtt_field	*tf;
	uint64_t			num_elems;

	tf = fn->fn_array_field;
	assert (tf != NULL);

	stbuf->st_nlink = 2;
	stbuf->st_mode = S_IFDIR | 0500;
	stbuf->st_size = ti_field_size(fn->fn_array_parent, tf, &num_elems)/8;
	stbuf->st_blksize = 512;
	return 0;
}

static int fslfuse_getattr_prim(struct fsl_fuse_node* fn, struct stat *stbuf)
{
	const struct fsl_rtt_field	*tf;
	uint64_t			num_elems;

	tf = fn->fn_field;
	assert (tf != NULL);

	stbuf->st_nlink = 1;
	stbuf->st_mode = S_IFREG | 0600;
	stbuf->st_size = ti_field_size(fn->fn_parent, tf, &num_elems)/8;
	stbuf->st_blksize = 512;
	return 0;
}

static int fslfuse_getattr_ti(const char *path, struct stat *stbuf)
{
	int			ret;
	struct fsl_fuse_node	*fn;

	fn = fslnode_by_path(path);
	if (fn == NULL) return -ENOENT;

	stbuf->st_uid = our_uid;
	stbuf->st_gid = our_gid;
	if (fn_is_type(fn)) {
		ret = fslfuse_getattr_type(fn->fn_ti, stbuf);
	} else if (fn_is_prim(fn)) {
		ret = fslfuse_getattr_prim(fn, stbuf);
	} else if (fn_is_array(fn)) {
		fprintf(out_file, "ARRAY! %s\n", path);
		ret = fslfuse_getattr_array(fn, stbuf);
		fprintf(out_file, "ARRAY DONE! %s\n", path);
	} else {
		assert (0 == 1);
	}

	stbuf->st_atime = open_time;
	stbuf->st_mtime = open_time;
	stbuf->st_ctime = open_time;

	fslnode_free(fn);

	return ret;
}

static int fslfuse_getattr(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}

	return fslfuse_getattr_ti(path, stbuf);
}

static int fslfuse_fgetattr(
	const char * path, struct stat * s, struct fuse_file_info * fi)
{
	fprintf(out_file, "FFFFFGETATTR %s %d\n", path, our_uid);
	fflush(out_file);

	return 0;
}

static int read_ti_dir(
	void *buf, fuse_fill_dir_t filler, off_t offset,
	struct type_info* ti)
{
	const struct fsl_rtt_type*	tt;
	unsigned int			i;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_fieldall_c; i++) {
		struct type_info		*cur_ti;
		const struct fsl_rtt_field	*tf;

		tf = &tt->tt_fieldall_thunkoff[i];
		cur_ti = typeinfo_follow_field(ti, tf);
		if (cur_ti == NULL) continue;

		if (filler(buf, tf->tf_fieldname, NULL, 0) == 1) {
			fprintf(out_file, "ARRRRRRGH FULL BUFF\n");
			fflush(out_file);
		}
	}

	return 0;
}

static int fslfuse_readdir(
	const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, struct fuse_file_info *fi)
{
	struct fsl_fuse_node	*fn;

	fprintf(out_file, "READDIR %s\n", path);
	fflush(out_file);

	fn = get_fnode(fi);
	if (fn_is_type(fn))
		return read_ti_dir(buf, filler, offset, get_fnode(fi)->fn_ti);
	else if (fn_is_array(fn)) {
	} else if (fn_is_prim(fn)) {
	}

	fprintf(out_file, "READDIR %s OK.\n", path);
	fflush(out_file);

	return -ENOENT;
}


static int fslfuse_close(const char *path, struct fuse_file_info *fi)
{
	if (!fi->fh) {
		fprintf(out_file, "fffffffff%s\n", path);
	}else {
		fprintf(out_file, "RELEASING %s\n", path);
		fslnode_free(get_fnode(fi));
	}

	fflush(out_file);
	return 0;
}


static int fslfuse_opendir(const char *path, struct fuse_file_info *fi)
{
	struct fsl_fuse_node	*fn;

	fprintf(out_file, "OD %s\n", path);
	fflush(out_file);

	fn = fslnode_by_path(path);
	if (fn == NULL) return -ENOENT;

	set_fnode(fi, fn);

	return 0;
}

static int fslfuse_open(const char *path, struct fuse_file_info *fi)
{
	fprintf(out_file, "O %s\n", path);
	fflush(out_file);

	if ((fi->flags & 3) != O_RDONLY) return -EACCES;

	return 0;
}

static int fslfuse_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;

	fprintf(out_file, "AAAAA %s\n", path);
	fflush(out_file);
	return -ENOENT;

	len = strlen(fslfuse_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, fslfuse_str + offset, size);
	} else
		size = 0;

	return size;
}

static int fslfuse_access(const char* path, int i)
{
	fprintf(out_file, "CCCCCC %s\n", path);
	fflush(out_file);
	return F_OK|R_OK|W_OK|X_OK;
}

static struct fuse_operations fslfuse_oper = {
	.getattr	= fslfuse_getattr,
	.readdir	= fslfuse_readdir,
	.opendir	= fslfuse_opendir,
	.open		= fslfuse_open,
	.release	= fslfuse_close,
	.releasedir	= fslfuse_close,
	.read		= fslfuse_read,
	.access		= fslfuse_access,
	.fgetattr	= fslfuse_fgetattr
};

int tool_entry(int argc, char *argv[])
{
	char	*args[3] = {"fusebrowse", "-s", argv[0]};

	assert (argc == 1 && "NEEDS MOUNT POINT");
	open_time = time(0);
	out_file = fopen("fusebrowse.err", "w");
	our_gid = getgid();
	our_uid = getuid();
	assert (out_file != NULL);
	return fuse_main(3, args, &fslfuse_oper);
}
