#define FUSE_USE_VERSION 25
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include "runtime.h"
#include "type_info.h"
#include "fuse_node.h"
#include "io.h"

FILE			*out_file;
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

	fprintf(out_file, "GETATTR GUY %s\n", path);
	fflush(out_file);

	fn = fslnode_by_path(path);
	if (fn == NULL) return -ENOENT;

	stbuf->st_uid = our_uid;
	stbuf->st_gid = our_gid;
	if (fn_is_type(fn)) {
		ret = fslfuse_getattr_type(fn->fn_ti, stbuf);
	} else if (fn_is_prim(fn)) {
		ret = fslfuse_getattr_prim(fn, stbuf);
	} else if (fn_is_array(fn)) {
		ret = fslfuse_getattr_array(fn, stbuf);
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
	const char * path, struct stat * stbuf, struct fuse_file_info * fi)
{
	struct fsl_fuse_node	*fn;

	fn = get_fnode(fi);
	if (fn == NULL) return -ENOENT;

	stbuf->st_uid = our_uid;
	stbuf->st_gid = our_gid;
	if (fn_is_type(fn)) {
		fslfuse_getattr_type(fn->fn_ti, stbuf);
	} else if (fn_is_prim(fn)) {
		fslfuse_getattr_prim(fn, stbuf);
	} else if (fn_is_array(fn)) {
		fslfuse_getattr_array(fn, stbuf);
	} else {
		assert (0 == 1);
	}

	stbuf->st_atime = open_time;
	stbuf->st_mtime = open_time;
	stbuf->st_ctime = open_time;

	return 0;
}

static int read_ti_dir(
	void *buf, fuse_fill_dir_t filler, struct type_info* ti)
{
	const struct fsl_rtt_type*	tt;
	struct type_info		*cur_ti;
	unsigned int			i;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_fieldall_c; i++) {
		const struct fsl_rtt_field	*tf;

		tf = &tt->tt_fieldall_thunkoff[i];
		if (tf->tf_cond != NULL) {
			if (tf->tf_cond(&ti_clo(ti)) == false) continue;
		}

		if (filler(buf, tf->tf_fieldname, NULL, 0) == 1) {
			fprintf(out_file, "ARRRRRRGH FULL BUFF\n");
			fflush(out_file);
		}
	}

	for (i = 0; i < tt->tt_pointsto_c; i++) {
		if (tt->tt_pointsto[i].pt_name == NULL) continue;
		cur_ti = typeinfo_follow_pointsto(ti, &tt->tt_pointsto[i], 0);
		if (cur_ti == NULL) continue;

		if (filler(buf, tt->tt_pointsto[i].pt_name, NULL, 0) == 1) {
			fprintf(out_file, "ALGRG FULLBUFF\n");
			fflush(out_file);
		}
		typeinfo_free(cur_ti);
	}

	for (i = 0; i < tt->tt_virt_c; i++) {
		int	err;
		if (tt->tt_virt[i].vt_name == NULL) continue;
		cur_ti = typeinfo_follow_virt(ti, &tt->tt_virt[i], 0, &err);
		if (cur_ti == NULL) continue;

		if (filler(buf, tt->tt_virt[i].vt_name, NULL, 0) == 1) {
			fprintf(out_file, "ALGRG FULLBUFF\n");
			fflush(out_file);
		}
		typeinfo_free(cur_ti);
	}

	return 0;
}

static int read_array_dir(
	void *buf, fuse_fill_dir_t filler, struct fsl_fuse_node* fn)
{
	unsigned int			i;
	uint64_t			elem_c;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	elem_c = fn->fn_array_field->tf_elemcount(&ti_clo(fn->fn_array_parent));
	for (i = 0; i < elem_c; i++) {
		char				name[16];
		struct type_info		*cur_ti;

		cur_ti = typeinfo_follow_field_off_idx(
			fn->fn_array_parent,
			fn->fn_array_field,
			ti_field_off(
				fn->fn_array_parent,
				fn->fn_array_field,
				i),
			i);
		if (cur_ti == NULL) continue;
		typeinfo_free(cur_ti);

		sprintf(name, "%d", i);
		if (filler(buf, name, NULL, 0) == 1) {
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
	if (fn_is_type(fn)) {
		return read_ti_dir(buf, filler, get_fnode(fi)->fn_ti);
	} else if (fn_is_array(fn)) {
		return read_array_dir(buf, filler, get_fnode(fi));
	} else if (fn_is_prim(fn)) {
		return -ENOENT;
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
		fprintf(out_file, "DONE RELEASING %s\n", path);
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
	if (fn_is_prim(fn)) {
		fslnode_free(fn);
		return -ENOENT;
	}

	set_fnode(fi, fn);

	return 0;
}

static int fslfuse_open(const char *path, struct fuse_file_info *fi)
{
	struct fsl_fuse_node	*fn;

	fprintf(out_file, "OP %s\n", path);
	fflush(out_file);

	if ((fi->flags & 3) != O_RDONLY) return -EACCES;
	fn = fslnode_by_path(path);
	if (fn == NULL) return -ENOENT;

	if (!fn_is_prim(fn)) {
		fslnode_free(fn);
		return -ENOENT;
	}

	set_fnode(fi, fn);

	return 0;
}

static int fslfuse_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	struct fsl_fuse_node	*fn;
	uint64_t		num_elems;
	size_t 			len;
	int			i;

	fn = get_fnode(fi);
	if (fn == NULL) return -ENOENT;
	if (!fn_is_prim(fn)) return -ENOENT;

	len = ti_field_size(fn->fn_parent, fn->fn_field, &num_elems)/8;

	if (offset < len) {
		diskoff_t	clo_off;

		if (offset + size > len) size = len - offset;

		clo_off = ti_offset(fn->fn_prim_ti);

		/* XXX SLOW */
		for (i = 0; i < size; i++) {
			char	c;
			c = __getLocal(
				&ti_clo(fn->fn_prim_ti),
				clo_off+(offset+i)*8,
				8);
			buf[i] = c;
		}
	} else
		size = 0;

	return size;
}

static int fslfuse_access(const char* path, int i)
{
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
