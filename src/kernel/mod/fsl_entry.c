#include <linux/fs.h>
#include <linux/file.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include "runtime.h"
#include "io.h"
#include "fsl_ioctl.h"
#include "alloc.h"

MODULE_LICENSE("BSD");	/* fuck the haters */
MODULE_AUTHOR("ANTONY ROMANO");

#define FSL_DEVNAME		"fslctl"

static bool is_inited = false;

extern uint64_t fsl_num_types;

static int init_rt(unsigned long fd)
{
	struct fsl_rt_ctx	*fsl_ctx;
	struct fsl_rt_io	*fsl_io;

	fsl_io = fsl_io_alloc_fd(fd);
	if (fsl_io == NULL) return -EBADF;

	fsl_ctx = fsl_alloc(sizeof(struct fsl_rt_ctx));
	if (fsl_ctx == NULL) {
		fsl_io_free(fsl_io);
		return -ENOMEM;
	}
	memset(fsl_ctx, 0, sizeof(struct fsl_rt_ctx));

	fsl_ctx->fctx_io = fsl_io;
	fsl_ctx->fctx_num_types = fsl_num_types;
	memset(&fsl_ctx->fctx_stat, 0, sizeof(struct fsl_rt_stat));
	fsl_vars_from_env(fsl_ctx);

	fsl_env = fsl_ctx;
	fsl_load_memo();

	return 0;
}

static void uninit_rt(void)
{
	if (is_inited == false) return;
	fsl_rt_uninit(fsl_env);
	is_inited = false;
}

static long fsl_ioctl_blkdevput(void)
{
	if (is_inited == false) return -ENXIO;
	uninit_rt();
	return 0;
}

static long fsl_ioctl_blkdevget(struct file* filp, unsigned long fd)
{
	int		ret;

	/* one blkdev at a time, please! */
	if (is_inited) return -EBUSY;

	ret = init_rt(fd);
	if (ret == 0) is_inited = true;

	return ret;
}

extern int scatter_entry(int argc, char* argv[]);
extern int defrag_entry(int argc, char* argv[]);
extern int smush_entry(int argc, char* argv[]);

static long fsl_ioctl_dotool(unsigned long arg)
{
	int	ret;

	if (is_inited == false) return -ENXIO;

	/* do the tool stuff here */
	switch (arg) {
	case FSL_DOTOOL_SCATTER: ret = scatter_entry(0, NULL); break;
	case FSL_DOTOOL_DEFRAG: ret = defrag_entry(0, NULL); break;
	case FSL_DOTOOL_SMUSH: ret = smush_entry(0, NULL); break;
	default: return -EINVAL;
	}

	return ret;
}

static int fsl_dev_open(struct inode* ino, struct file* f) { return 0; }
static int fsl_dev_release(struct inode* ino, struct file* f) { return 0; }
static long fsl_dev_ioctl(
	struct file* filp, unsigned int ioctl, unsigned long arg)
{
	long ret = -ENOSYS;
	switch (ioctl) {
	case FSL_IOCTL_BLKDEVGET:
		ret = fsl_ioctl_blkdevget(filp, arg);
		break;
	case FSL_IOCTL_BLKDEVPUT: ret = fsl_ioctl_blkdevput(); break;
	case FSL_IOCTL_DOTOOL: ret = fsl_ioctl_dotool(arg); break;
	default: printk("WHHHHHHAT %x\n", ioctl);
	}
	return ret;
}

static struct file_operations fsl_dev_fops = {
	.unlocked_ioctl		= fsl_dev_ioctl,
	.open			= fsl_dev_open,
	.release		= fsl_dev_release,
	.owner			= THIS_MODULE,
};

static struct miscdevice fsl_dev =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = FSL_DEVNAME,
	.fops = &fsl_dev_fops
};

/* TODO: create ioctl files! */
static int __init init_fsl_mod(void)
{
	int	err;

	err = misc_register(&fsl_dev);
	if (err) {
		printk(KERN_ERR "fsl: misc dev register failed\n");
		goto done;
	}
	printk("fsl: Hello.\n");
done:
	return err;
}

static void __exit exit_fsl_mod(void)
{
	int	err;

	uninit_rt();
	err = misc_deregister(&fsl_dev);
	if (err)
		printk(KERN_ERR "fsl: failed to unregister misc dev\n");
	printk("fsl: Goodbye.\n");
}

module_init(init_fsl_mod)
module_exit(exit_fsl_mod)
