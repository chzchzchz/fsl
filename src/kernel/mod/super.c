#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include "runtime.h"
#include "tool.h"
#include "debug.h"
#include "info.h"
#include "rt.h"
#include "inode.h"
#include "type_info.h"
#include "bridge.h"

extern uint64_t fsl_num_types;

static void fsl_put_super(struct super_block *sb);
static void fsl_destroy_inode(struct inode* inode);
static int fsl_sync_fs(struct super_block* sb, int wait);
static int fsl_show_options(struct seq_file* seq, struct vfsmount* vfs);
static int fsl_remount(struct super_block* sb, int * flags, char* data);
static const struct super_operations fsl_super_ops =
{
	.destroy_inode	= fsl_destroy_inode,
	.put_super	= fsl_put_super,
	.sync_fs	= fsl_sync_fs,
	.show_options	= fsl_show_options,
	.remount_fs	= fsl_remount,
};

static int fsl_type_getattr(
	struct vfsmount *mnt, struct dentry *dentry, struct kstat *stat)
{
	return 0;
}

/** writes out super data, frees super block memory */
static void fsl_put_super(struct super_block *sb) {}

/** sync super data */
static void fsl_write_super(struct super_block *sb) {}

static int fsl_remount(struct super_block* sb, int * flags, char* data)
{
	return -EINVAL;
}

static int fsl_sync_fs(struct super_block* sb, int wait)
{
	return -ENOSYS;
}

static int fsl_show_options(struct seq_file* seq, struct vfsmount* vfs)
{
	return -ENOSYS;
}

/** destroy fs private structures for inode */
static void fsl_destroy_inode(struct inode* inode)
{
	fsl_bridge_free(fbn_by_ino(inode));
	inode->i_private = NULL;
}

/** Called at last iput if i_nlink is zero */
static void fsl_delete_inode(struct inode* inode) { /* XXX */ }

static int fsl_fill_super(struct super_block* sb, void* data, int silent)
{
	int			err;
	struct type_info	*ti_origin;
	struct fsl_bridge_node	*fbn;

	/* loads up superblock */
	DEBUG_WRITE("FILL SUPER.");
	sb->s_flags |= MS_RDONLY;
	sb->s_fs_info = NULL;	/* env data? */
	if (sb->s_bdev == NULL) return -ENXIO;

	DEBUG_WRITE("BDEV INIT");
	if ((err = kfsl_rt_init_bdev(sb->s_bdev)) != 0) {
		DEBUG_WRITE("INIT BDEV FAILURE");
		return err;
	}

	DEBUG_WRITE("GRABBING ORIGIN.");
	ti_origin = typeinfo_alloc_origin();
	if (ti_origin == NULL) {
		DEBUG_WRITE("BAD ORIGIN");
		kfsl_rt_uninit();
		return -EINVAL;
	}
	fbn = fsl_bridge_from_ti(ti_origin);
	if (fbn == NULL) {
		typeinfo_free(ti_origin);
		return -EINVAL;
	}

	DEBUG_WRITE("SETTING ROOT");
	sb->s_op = &fsl_super_ops;
	sb->s_root = d_alloc_root(fsl_inode_iget(sb, fbn));
	if (IS_ERR(sb->s_root)) {
		DEBUG_WRITE("BAD ALLOC");
		fsl_bridge_free(fbn);
		kfsl_rt_uninit();
		return PTR_ERR(sb->s_root);
	}

	DEBUG_WRITE("OK. KEEP ON");
	return 0;
}

static int fslfs_get_sb(struct file_system_type *fs_type,
	int flags, const char *dev_name, void* data, struct vfsmount* mnt)
{
	int	err;
//	DEBUG_WRITE("GET SB BDEV??");
	err = get_sb_bdev(fs_type, flags, dev_name, data, fsl_fill_super, mnt);
//	DEBUG_WRITE("SB_BDEV=%d", err);
	return err;
}

static struct file_system_type fsl_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "fslfs-ext2",
	.get_sb		= fslfs_get_sb,
	.kill_sb	= kill_block_super,
	.fs_flags	= FS_REQUIRES_DEV
};

TOOL_ENTRY(kernbrowser)
{
	DEBUG_WRITE("INIT FS REGISTER...");
	FSL_ASSERT(fsl_num_types < 255);

	return register_filesystem(&fsl_fs_type);
}

int kernbrowser_exit(void)
{
	DEBUG_WRITE("BYE BYE FS.");
	return unregister_filesystem(&fsl_fs_type);
}
