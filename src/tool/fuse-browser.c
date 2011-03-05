#define FUSE_USE_VERSION 25
//#define DEBUG_FUSE
#include <fuse.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include "debug.h"
#include "runtime.h"
#include "type_info.h"
#include "fuse_node.h"
#include "io.h"
#include "tool.h"

FILE			*out_file;
static uid_t		our_uid;
static gid_t		our_gid;
time_t			open_time;

#define get_fnode(x)	((struct fsl_bridge_node*)(((x))->fh))
#define set_fnode(x,y)	((x))->fh = (uint64_t)((void*)(y))


struct array_ops
{
	uint64_t (*ao_elems)(struct fsl_bridge_node* fbn);
	struct type_info* (*ao_get)(struct fsl_bridge_node* fbn, unsigned int);
};

static int fslfuse_getattr_type(struct type_info* ti, struct stat* stbuf)
{
	stbuf->st_mode = S_IFDIR | 0500;
	stbuf->st_nlink = 2;
	stbuf->st_size = ti_size(ti) / 8;
	stbuf->st_blksize = 512;
	return 0;
}

static int fslfuse_getattr_array(
	struct fsl_bridge_node* fbn, struct stat* stbuf)
{
	stbuf->st_nlink = 2;
	stbuf->st_mode = S_IFDIR | 0500;
	stbuf->st_blksize = 512;
	stbuf->st_size = fbn_bytes(fbn);
	return 0;
}

static int fslfuse_getattr_prim(struct fsl_bridge_node* fbn, struct stat *stbuf)
{
	stbuf->st_nlink = 1;
	stbuf->st_mode = S_IFREG | 0600;
	stbuf->st_blksize = 512;
	stbuf->st_size = fbn_bytes(fbn);
	return 0;
}

static int fslfuse_getattr_fn(struct fsl_bridge_node* fbn, struct stat * stbuf)
{
	stbuf->st_uid = our_uid;
	stbuf->st_gid = our_gid;
	if (fbn_is_type(fbn)) {
		fslfuse_getattr_type(ti_by_fbn(fbn), stbuf);
	} else if (fbn_is_prim(fbn)) {
		fslfuse_getattr_prim(fbn, stbuf);
	} else if (fbn_is_array(fbn)) {
		fslfuse_getattr_array(fbn, stbuf);
	} else {
		DEBUG_WRITE("TYPE: %d", fsl_bridge_type(fbn));
		assert (0 == 1);
	}

	stbuf->st_atime = open_time;
	stbuf->st_mtime = open_time;
	stbuf->st_ctime = open_time;

	return 0;
}

static int fslfuse_getattr_ti(const char *path, struct stat *stbuf)
{
	int			ret;
	struct fsl_bridge_node	*fbn;

	fbn = fslnode_by_path(path);
	if (fbn == NULL) return -ENOENT;
	ret = fslfuse_getattr_fn(fbn, stbuf);

	fsl_bridge_free(fbn);

	return ret;
}

static int fslfuse_getattr(const char *path, struct stat *stbuf)
{
	DEBUG_WRITE("GETATTR %s", path);
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
	struct fsl_bridge_node	*fbn;

	fbn = get_fnode(fi);
	if (fbn == NULL) return -ENOENT;

	fslfuse_getattr_fn(fbn, stbuf);

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
		const struct fsl_rtt_pointsto*	pt = &tt->tt_pointsto[i];

		if (pt->pt_name == NULL) continue;
		cur_ti = typeinfo_follow_pointsto(
			ti, pt, pt->pt_iter.it_min(&ti_clo(ti)));
		if (cur_ti == NULL) continue;

		if (filler(buf, pt->pt_name, NULL, 0) == 1)
			DEBUG_WRITE("ALGRG FULLBUFF PT");
		typeinfo_free(cur_ti);
	}

	for (i = 0; i < tt->tt_virt_c; i++) {
		const struct fsl_rtt_virt	*vt = &tt->tt_virt[i];
		int				err;

		if (vt->vt_name == NULL) continue;
		cur_ti = typeinfo_follow_virt(ti, vt, 0, &err);
		if (cur_ti == NULL) continue;

		if (filler(buf, vt->vt_name, NULL, 0) == 1)
			DEBUG_WRITE("ALGRG FULLBUFF VT");
		typeinfo_free(cur_ti);
	}

	return 0;
}

#define TYPEINFO_NOMORE		((void*)((intptr_t)~0))
#define ti_is_nomore(x)		((x) == TYPEINFO_NOMORE)

static uint64_t ao_elems_field(struct fsl_bridge_node* fbn)
{
	return fbn->fbn_arr_field->tf_elemcount(
		&ti_clo(fbn->fbn_arr_parent));
}

static struct type_info* ao_get_field(struct fsl_bridge_node* fbn, unsigned int i)
{
	ti_field_off(fbn->fbn_arr_parent, fbn->fbn_arr_field, i);
	return typeinfo_follow_field_off_idx(
			fbn->fbn_arr_parent, fbn->fbn_arr_field,
			ti_field_off(fbn->fbn_arr_parent, fbn->fbn_arr_field, i),
			i);
}

static uint64_t ao_elems_pt(struct fsl_bridge_node* fbn)
{
	uint64_t	it_min, it_max;
	it_min = fbn->fbn_arr_pt->pt_iter.it_min(&ti_clo(fbn->fbn_arr_parent));
	if (it_min == ~0) return 0;
	it_max = fbn->fbn_arr_pt->pt_iter.it_max(&ti_clo(fbn->fbn_arr_parent));
	if (it_min > it_max) return 0;
	return (it_max-it_min)+1;
}

static struct type_info* ao_get_pt(struct fsl_bridge_node* fbn, unsigned int i)
{
	uint64_t	min;
	min = fbn->fbn_arr_pt->pt_iter.it_min(&ti_clo(fbn->fbn_arr_parent));
	if (min == ~0) return NULL;
	return typeinfo_follow_pointsto(
		fbn->fbn_arr_parent, fbn->fbn_arr_pt, i+min);
}

static uint64_t ao_elems_vt(struct fsl_bridge_node* fbn) { return ~0; }

static struct type_info* ao_get_vt(struct fsl_bridge_node* fbn, unsigned int i)
{
	int			err;
	struct type_info	*ti;

	ti = typeinfo_follow_virt(fbn->fbn_arr_parent, fbn->fbn_arr_vt, i, &err);
	if (ti) return ti;
	if (err == TI_ERR_BADVERIFY) return NULL;
	return TYPEINFO_NOMORE;
}

struct array_ops ao_field_ops = {
	.ao_elems = ao_elems_field, .ao_get = ao_get_field};
struct array_ops ao_pt_ops = { .ao_elems = ao_elems_pt, .ao_get = ao_get_pt};
struct array_ops ao_vt_ops = { .ao_elems = ao_elems_vt, .ao_get = ao_get_vt};

static int read_array_dir(
	void *buf, fuse_fill_dir_t filler,
	struct fsl_bridge_node* fbn,
	struct array_ops* ao)
{
	unsigned int			i;
	uint64_t			elem_c;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	elem_c = ao->ao_elems(fbn);
	for (i = 0; i < elem_c; i++) {
		char				name[256];
		struct type_info		*cur_ti;

		cur_ti = ao->ao_get(fbn, i);
		if (cur_ti == NULL) {
			continue;
		} else if (cur_ti == TYPEINFO_NOMORE)
			break;

		if (typeinfo_getname(cur_ti, name, 256) == NULL)
			sprintf(name, "%d", i);

		typeinfo_free(cur_ti);

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
	struct fsl_bridge_node	*fbn;

	DEBUG_WRITE("READDIR %s", path);

	fbn = get_fnode(fi);
	if (fbn_is_type(fbn)) {
		return read_ti_dir(buf, filler, get_fnode(fi)->fbn_ti);
	} else if (fbn_is_array_field(fbn)) {
		return read_array_dir(buf, filler, get_fnode(fi), &ao_field_ops);
	} else if (fbn_is_array_pt(fbn)) {
		return read_array_dir(buf, filler, get_fnode(fi), &ao_pt_ops);
	} else if (fbn_is_array_vt(fbn)) {
		return read_array_dir(buf, filler, get_fnode(fi), &ao_vt_ops);
	}else if (fbn_is_prim(fbn)) {
		// fprintf(out_file, "ENOENT\n"); fflush(out_file);
		return -ENOENT;
	}

	return -ENOENT;
}

static int fslfuse_close(const char *path, struct fuse_file_info *fi)
{
	if (fi->fh) fsl_bridge_free(get_fnode(fi));
	return 0;
}

static int fslfuse_opendir(const char *path, struct fuse_file_info *fi)
{
	struct fsl_bridge_node	*fbn;

	DEBUG_WRITE("OD %s", path);

	fbn = fslnode_by_path(path);
	if (fbn == NULL) return -ENOENT;
	if (fbn_is_prim(fbn)) {
		fsl_bridge_free(fbn);
		return -ENOENT;
	}

	set_fnode(fi, fbn);

	return 0;
}

static int fslfuse_open(const char *path, struct fuse_file_info *fi)
{
	struct fsl_bridge_node	*fbn;

	if ((fi->flags & 3) != O_RDONLY) return -EACCES;
	fbn = fslnode_by_path(path);
	if (fbn == NULL) return -ENOENT;

	if (!fbn_is_prim(fbn)) {
		fsl_bridge_free(fbn);
		return -ENOENT;
	}

	set_fnode(fi, fbn);

	return 0;
}

static int fslfuse_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
	struct fsl_bridge_node	*fbn;
	diskoff_t		clo_off;
	size_t 			len;
	int			i;

	fbn = get_fnode(fi);
	if (fbn == NULL) return -ENOENT;
	if (!fbn_is_prim(fbn)) return -ENOENT;

	len = fbn_bytes(fbn);
	if (offset >= len) return 0;

	assert (offset < len);

	clo_off = ti_offset(fbn->fbn_prim_ti);
	if ((size + offset) > len) size = len - offset;

	/* XXX SLOW-- adapt ti_to_buf to do offsets */
	for (i = 0; i < size; i++) {
		buf[i] = __getLocal(
			&ti_clo(fbn->fbn_prim_ti), clo_off+(offset+i)*8, 8);
	}

	return size;
}

static int fslfuse_access(const char* path, int i)
{
	return F_OK;
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

TOOL_ENTRY(fusebrowser)
{
#ifdef DEBUG_FUSE
#define NUM_ARGS	4
	char	*args[4] = {"fusebrowse", "-s", "-d", argv[0]};
#else
#define NUM_ARGS	3
	char	*args[3] = {"fusebrowse", "-s", argv[0]};
#endif
	assert (argc == 1 && "NEEDS MOUNT POINT");
	open_time = time(0);
	out_file = fopen("fusebrowse.err", "w");
	if (out_file != NULL) {
		setlinebuf(out_file);
		fsl_debug_set_file(out_file);
	}
	our_gid = getgid();
	our_uid = getuid();
	assert (out_file != NULL);
	return fuse_main(NUM_ARGS, args, &fslfuse_oper);
}
