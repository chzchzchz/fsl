#include <linux/blkdev.h>
#include <linux/fs.h>

#if 0
static const struct file_operations fsl_file_ops = {
	.llseek = generic_file_llseek,
	.read = do_sync_read,
	.aio_read =  generic_file_aio_read,
	.splice_read = generic_file_splice_read,
	.write = fsl_file_write,
	.mmap = fsl_file_mmap,
	.open = generic_file_open,
	.release = fsl_release_file,
	.fsync = fsl_sync_file,
	.unlocked_ioctl =  fsl_ioctl
};

struct const file_operations fsl_dir_file_ops = {
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.readdir	= fsl_real_readdir,
	.unlocked_ioctl	= fsl_ioctl,
	.release	= fsl_release_file,
	.fsync		= fsl_sync_file
};

static const struct inode_operations fsl_file_inode_operations =
{
	.truncate       = fsl_truncate,
	.getattr        = fsl_getattr,
	.setattr        = fsl_setattr,
	.setxattr       = fsl_setxattr,
	.getxattr       = fsl_getxattr,
	.listxattr      = fsl_listxattr,
	.removexattr    = fsl_removexattr,
	.permission     = fsl_permission,
	.fallocate      = fsl_fallocate,
	.fiemap         = fsl_fiemap,
};

static const struct inode_operations fsl_dir_inode_operations =
{
	.getattr        = fsl_getattr,
	.lookup         = fsl_lookup,
	.create         = fsl_create,
	.unlink         = fsl_unlink,
	.link           = fsl_link,
	.mkdir          = fsl_mkdir,
	.rmdir          = fsl_rmdir,
	.rename         = fsl_rename,
	.symlink        = fsl_symlink,
	.setattr        = fsl_setattr,
	.mknod          = fsl_mknod,
	.setxattr       = fsl_setxattr,
	.getxattr       = fsl_getxattr,
	.listxattr      = fsl_listxattr,
	.removexattr    = fsl_removexattr,
	.permission     = fsl_permission,
};
#endif

/**
 * feed data from sb into stat buf..
 */
static int fsl_statfs(struct dentry* dentry, struct kstatfs* buf)
{
}

/**
 * writes out super data, frees super block memory
 */
static void fsl_put_super(struct super_block *sb)
{
}

/**
 * sync super data
 */
static void fsl_write_super(struct super_block *sb)
{
}

static int fsl_remount(struct super_block* sb, int * flags, char* data)
{
	return -EINVAL;
}

static int fsl_sync_fs(struct super_block* sb, int wait)
{
}

static int fsl_show_options(struct seq_file* seq, struct vfsmount* vfs)
{
	return -ENOSYS;
}

/**
 * writes inode data to disk
 */
static int fsl_write_inode(struct inode* inode, int do_sync)
{
}

/**
 * allocate fs private structures for inode
 */
static struct inode* fsl_alloc_inode(struct super_block* sb)
{
}

/**
 * destroy fs private structures for inode
 */
static void fsl_destroy_inode(struct inode* inode)
{
}

/**
 * Called at last iput if i_nlink is zero
 */
static void fsl_delete_inode(struct inode* inode)
{
	/* XXX */
}

static long fsl_ioctl(
	struct file* filp, unsigned int cmd, unsigned long arg)
{
	return -ENOTTY;
}

static int fsl_fill_super(struct super_block* sb, void* data, int silent)
{
	/* loads up superblock */
	/* XXX */
}

static int fsl_get_sb(struct file_system_type *fs_type,
	int flags, const char *dev_name, void* data, struct vfsmount* mnt)
{
	return get_sb_bdev(fs_type, flags, dev_name, data, fsl_fill_super, mnt);
}

static const struct file_system_type fsl_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "fsl0000",
	.get_sb		= fsl_get_sb,
	.kill_sb	= kill_block_super,
	.fs_flags	= FS_REQUIRES_DEV
};

static const struct super_operations fsl_super_ops =
{
	.alloc_inode	= fsl_alloc_inode,
	.destroy_inode	= fsl_destroy_inode,
//	.delete_inode	= fsl_delete_inode,
	.put_super	= fsl_put_super,
	.write_super	= fsl_write_super,
	.sync_fs	= fsl_sync_fs,
	.show_options	= fsl_show_options,
	.write_inode	= fsl_write_inode,
	.statfs		= fsl_statfs,
	.remount_fs	= fsl_remount,
};
