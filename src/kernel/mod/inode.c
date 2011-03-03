#define DEBUG_TOOL
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/namei.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include "type_info.h"
#include "bridge.h"
#include "debug.h"
#include "inode.h"
#include "alloc.h"
#include "io.h"

struct file_priv
{
	loff_t			fp_realpos;
	struct type_info	*fp_ti;
};

static struct dentry* fsl_type_lookup(
	struct inode *, struct dentry *, struct nameidata *);
static struct dentry* fsl_array_lookup(
	struct inode *dir, struct dentry *de, struct nameidata *nid);
static struct dentry* fsl_pt_lookup(
	struct inode *dir, struct dentry *de, struct nameidata *nid);
static struct dentry* fsl_vt_lookup(
	struct inode *dir, struct dentry *de, struct nameidata *nid);

/* readdir ops exposed to kernel */
static int fsl_type_readdir(struct file *f, void *b, filldir_t fill);
static int fsl_array_readdir(struct file *f, void *b, filldir_t fill);
static int fsl_pt_readdir(struct file *f, void *b, filldir_t fill);
static int fsl_vt_readdir(struct file *f, void *b, filldir_t fill);
/* readdir helper funcs */
static int fsl_type_intrinsic_readdir(
	struct file *file, void *buf, filldir_t filler, int* err);
static int fsl_type_readdir_ti(struct file *file, void *buf, filldir_t filler);
static int fsl_array_readdir_ti(struct file *file, void *buf, filldir_t filler);
static int fsl_pt_readdir_ti(struct file *file, void *buf, filldir_t fill);
static int fsl_readdir_array_fbn(struct file *file, void *buf, filldir_t fill);

static int fsl_type_open(struct inode* inode, struct file* file);
static int fsl_type_release(struct inode *inode, struct file *file);

int fsl_prim_readpage(struct file *file, struct page *page);

#define file_get_priv(x)		((struct file_priv*)((x)->private_data))

static struct inode_operations fsl_typedir_inode_ops =
{	.lookup		= fsl_type_lookup };
static struct inode_operations fsl_prim_inode_ops =
{	.lookup		=  NULL };
static struct inode_operations fsl_array_inode_ops =
{	.lookup		= fsl_array_lookup };

static struct file_operations fsl_typedir_file_ops =
{
	.open		= fsl_type_open,
	.release	= fsl_type_release,
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.readdir	= fsl_type_readdir,
};

static struct file_operations fsl_prim_file_ops =
{
	.open		= fsl_type_open,
	.release	= fsl_type_release,
	.llseek		= generic_file_llseek,
	.aio_read       = generic_file_aio_read,
	.read		= do_sync_read,
	.mmap           = generic_file_readonly_mmap,
};

struct address_space_operations fsl_prim_aops = {
	.readpage       = fsl_prim_readpage,
};

static struct file_operations fsl_array_file_ops =
{
	.open		= fsl_type_open,
	.release	= fsl_type_release,
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.readdir	= fsl_array_readdir,
};

static struct file_operations fsl_pt_file_ops =
{
	.open		= fsl_type_open,
	.release	= fsl_type_release,
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.readdir	= fsl_pt_readdir,
};

static struct file_operations fsl_vt_file_ops =
{
	.open		= fsl_type_open,
	.release	= fsl_type_release,
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.readdir	= fsl_vt_readdir,
};

/* DT_REG, DT_DIR, DT_UNKNOWN.. */
/* TODO: use fbns */
static int fsl_type_readdir_ti(struct file *file, void *buf, filldir_t filler)
{
	loff_t				pos = file->f_pos - 2;
	loff_t				realpos;
	int				full, i;
	const struct fsl_rtt_type	*tt;
	struct type_info		*ti, *cur_ti;

	BUG_ON(pos < 0);
	DEBUG_TOOL_WRITE("READING DIR");

	realpos = file_get_priv(file)->fp_realpos;
	ti = ti_by_fbn(fbn_by_ino(file->f_dentry->d_inode));
	tt = tt_by_ti(ti);

	/* fields */
	for (i = realpos; i < tt->tt_fieldall_c; i++, realpos++) {
		const struct fsl_rtt_field      *tf;
		struct fsl_bridge_node		*fbn;

		tf = &tt->tt_fieldall_thunkoff[i];
		fbn = fsl_bridge_follow_tf(ti, tf);
		if (fbn == NULL) continue;

		full = filler(
			buf,
			tf->tf_fieldname,
			strlen(tf->tf_fieldname),
			pos++,
			-1, /* ino */
			typenum_to_dt(tf->tf_typenum));
		fsl_bridge_free(fbn);

		if (full) goto is_full;
	}
	i -= tt->tt_fieldall_c;

	/* points to */
	for (; i < tt->tt_pointsto_c; i++, realpos++) {
		const struct fsl_rtt_pointsto*  pt = &tt->tt_pointsto[i];

		if (pt->pt_name == NULL) continue;
		cur_ti = typeinfo_follow_pointsto(
			ti, pt, pt->pt_iter.it_min(&ti_clo(ti)));
		if (cur_ti == NULL) continue;

		full = filler(
			buf, pt->pt_name,
			strlen(pt->pt_name),
			pos++, -1, DT_DIR);
		typeinfo_free(cur_ti);
		if (full) goto is_full;
	}
	i -= tt->tt_pointsto_c;

	/* virt */
	for (; i < tt->tt_virt_c; i++, realpos++) {
		const struct fsl_rtt_virt       *vt = &tt->tt_virt[i];
		int				err;

		if (vt->vt_name == NULL) continue;
		cur_ti = typeinfo_follow_virt(ti, vt, 0, &err);
		if (cur_ti == NULL) continue;
		full = filler(
			buf, vt->vt_name,
			strlen(vt->vt_name), pos++, -1, DT_DIR);
		typeinfo_free(cur_ti);
		if (full) goto is_full;
	}

is_full:
	file_get_priv(file)->fp_realpos = realpos;
	file->f_pos = pos + 2;
	return 0;
}

static int fsl_type_intrinsic_readdir(
	struct file *file, void *buf, filldir_t filler, int* err)
{
	if (file->f_pos < 0) {
		*err = -EINVAL;
		return 0;
	}

	*err = 0;
	if (file->f_pos == 0) {
		struct inode *inode = file->f_dentry->d_inode;
		if (filler(buf, ".", 1, 1, inode->i_ino, DT_DIR) < 0)
			return 0;
		file->f_pos++;
	}

	if (file->f_pos == 1) {
		ino_t	pino = parent_ino(file->f_dentry);
		if (filler(buf, "..", 2, 2, pino, DT_DIR) < 0) return 0;
		file->f_pos++;
	}

	return 1;
}

static int fsl_type_readdir(struct file *file, void *buf, filldir_t filler)
{
	int	err;
	if (!fsl_type_intrinsic_readdir(file, buf, filler, &err))
		return err;
	return fsl_type_readdir_ti(file, buf, filler);
}

static int fsl_readdir_array_fbn(struct file *file, void *buf, filldir_t fill)
{
	loff_t				pos = file->f_pos - 2;
	struct fsl_bridge_node		*fbn;
	loff_t				realpos;
	int				full, i, err;

	BUG_ON(pos < 0);

	realpos = file_get_priv(file)->fp_realpos;

	fbn = fbn_by_ino(file->f_dentry->d_inode);
	i = realpos;
	while (1) {
		struct fsl_bridge_node		*cur_fbn;
		char				name[64];

		cur_fbn = fsl_bridge_idx_into(fbn, i, &err);
		if (cur_fbn == NULL) {
			if (err != TI_ERR_BADVERIFY) break;
			goto next;
		}

		if (typeinfo_getname(cur_fbn->fbn_ti, name, 64) == NULL)
			sprintf(name, "%d", i);
		fsl_bridge_free(cur_fbn);

		full = fill(buf, name, strlen(name), pos++, -1, DT_DIR);
		if (full) break;
next:
		i++;
		realpos++;
	}

	file_get_priv(file)->fp_realpos = realpos;
	file->f_pos = pos + 2;
	return 0;
}

static int fsl_array_readdir(struct file *file, void *buf, filldir_t fill)
{
	/* XXX STUB */
	int	err;
	if (!fsl_type_intrinsic_readdir(file, buf, fill, &err))
		return err;
	return fsl_readdir_array_fbn(file, buf, fill);
}

static int fsl_pt_readdir(struct file *file, void *buf, filldir_t fill)
{
	int	err;
	if (!fsl_type_intrinsic_readdir(file, buf, fill, &err))
		return err;
	return fsl_pt_readdir_ti(file, buf, fill);
}

static int fsl_vt_readdir(struct file *file, void *buf, filldir_t fill)
{
	/* XXX STUB */
	int	err;
	if (!fsl_type_intrinsic_readdir(file, buf, fill, &err))
		return err;
	return -ENOSYS;
}

static int fsl_type_open(struct inode* inode, struct file* file)
{
	struct file_priv	*priv;
	int			err;

	if ((err = generic_file_open(inode, file)) != 0)
		return err;

	priv = fsl_alloc(sizeof(struct file_priv));
	if (priv == NULL) return -ENOMEM;
	priv->fp_realpos = 0;
	file->private_data = priv;

	return 0;
}

static int fsl_type_release(struct inode *inode, struct file *file)
{
	struct file_priv	*priv = file_get_priv(file);
	fsl_free(priv);
	file->private_data = NULL;
	return 0;
}

static struct dentry* fsl_array_lookup(
	struct inode *dir, struct dentry *de, struct nameidata *nid)
{
	struct inode		*new_inode;
	struct fsl_bridge_node	*new_fbn = NULL;

	if (FSL_IS_INT(de->d_name.name)) {
		int	err;
		new_fbn = fsl_bridge_idx_into(
			fbn_by_ino(dir), FSL_ATOI(de->d_name.name), &err);
	} else {
/* XXX TODO */
//		new_fbn = fsl_bridge_follow_name(
//			ti_by_fbn(fbn_by_ino(dir)), de->d_name.name);
//		if (new_fbn == NULL) return NULL;
//
		return NULL;
	}
	if (new_fbn == NULL) return NULL;
	new_inode = fsl_inode_iget(dir->i_sb, new_fbn);
	return d_splice_alias(new_inode, de);
}

static struct dentry* fsl_type_lookup(
	struct inode *dir, struct dentry *de, struct nameidata *nid)
{
	struct inode		*new_inode;
	struct fsl_bridge_node	*new_fbn;

	new_fbn = fsl_bridge_follow_name(
		ti_by_fbn(fbn_by_ino(dir)), de->d_name.name);
	if (new_fbn == NULL) return NULL;

	new_inode = fsl_inode_iget(dir->i_sb, new_fbn);
	if (new_inode == NULL) return NULL;

	return d_splice_alias(new_inode, de);
}

int fsl_prim_readpage(struct file *file, struct page *page)
{
	int			err = 0;
	loff_t			pg_offset, read_size, offset;
	char			*buffer;
	struct fsl_bridge_node	*fbn;
	diskoff_t		clo_off;
	struct inode		*inode;
	unsigned int		i;

	inode = page->mapping->host;
	BUG_ON(!PageLocked(page));

	fbn = fbn_by_ino(inode);

	buffer = kmap(page);
	pg_offset = page_offset(page);

	offset = file->f_pos + pg_offset;

	/* nothing to read */
	if (offset >= inode->i_size) {
		memset(buffer, 0, PAGE_CACHE_SIZE-pg_offset);
		goto done;
	}

	read_size = PAGE_CACHE_SIZE;
	if ((read_size + offset) > inode->i_size)
		read_size = inode->i_size - offset;

	/* XXX SLOW-- adapt ti_to_buf to do offsets */
	clo_off = ti_offset(fbn->fbn_prim_ti);
	for (i = 0; i < read_size; i++) {
		buffer[i] = __getLocal(
			&ti_clo(fbn->fbn_prim_ti),
			clo_off+(offset+i)*8,
			8);
	}

	memset(buffer + read_size, 0, PAGE_CACHE_SIZE - read_size);
	flush_dcache_page(page);
	SetPageUptodate(page);

done:
	kunmap(page);
	unlock_page(page);
	return err;
}

/* inode is given ownership of 'ti' if successful */
struct inode *fsl_inode_iget(
	struct super_block *sb, struct fsl_bridge_node* fbn)
{
	struct inode		*inode;
	enum fsl_node_t		kind;

	inode = new_inode(sb);	/* TODO: iget5_locked */
	if (inode == NULL) return ERR_PTR(-ENOMEM);

	if (inode->i_nlink == 0) {
		/* set i_nlink to 0 to prevent caching */
		iget_failed(inode);
		return ERR_PTR(-ENOENT);
	}

	kind = fsl_bridge_type(fbn);
	ino_put_fbn(inode, fbn);
	inode->i_size = fbn_bytes(fbn);
	switch(kind) {
	case FSL_NODE_TYPE:
		inode->i_op = &fsl_typedir_inode_ops;
		inode->i_fop = &fsl_typedir_file_ops;
		inode->i_mode = S_IFDIR | 0555;
		break;
	case FSL_NODE_PRIM:
		inode->i_op = &fsl_prim_inode_ops;
		inode->i_fop = &fsl_prim_file_ops;
		inode->i_mode = S_IFREG | 0444;
		inode->i_mapping->a_ops = &fsl_prim_aops;
		break;
	case FSL_NODE_PRIMARRAY:
	case FSL_NODE_TYPEARRAY:
		inode->i_op = &fsl_array_inode_ops;
		inode->i_fop = &fsl_array_file_ops;
		inode->i_mode = S_IFDIR | 0555;
		break;
	case FSL_NODE_POINTSTO:
		inode->i_op = &fsl_array_inode_ops;
		inode->i_fop = &fsl_array_file_ops;
		inode->i_mode = S_IFDIR | 0555;
		break;
	case FSL_NODE_VIRT:
		inode->i_op = &fsl_array_inode_ops;
		inode->i_fop = &fsl_array_file_ops;
		inode->i_mode = S_IFDIR | 0555;
		break;
	default:
		DEBUG_WRITE("WTF?? kind=%d",kind);
		unlock_new_inode(inode);
		iput(inode);
		return ERR_PTR(-EINVAL);
	}

	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME_SEC;
	inode->i_uid = 0;
	inode->i_gid = 0;
	inode->i_version = 1;
	inode->i_blocks = (inode->i_size+sb->s_blocksize-1)/sb->s_blocksize;

	inode->i_ino = iunique(sb, 2);
	insert_inode_hash(inode);
	return inode;
}
