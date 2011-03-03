#include <linux/fs.h>
#include <linux/file.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include "fsl_ioctl.h"
#include "rt.h"

MODULE_LICENSE("GPL");	/* necessary for sync_filesystem. asshats */
MODULE_AUTHOR("ANTONY ROMANO");

#define FSL_DEVNAME		"fslctl"

extern int defrag_entry(int argc, char* argv[]);
extern int smush_entry(int argc, char* argv[]);
extern int scatter_entry(int argc, char* argv[]);

static long fsl_ioctl_dotool(unsigned long arg)
{
	int	ret;

	if (kfsl_rt_inited() == false) return -ENXIO;

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
	case FSL_IOCTL_BLKDEVGET: ret = kfsl_rt_init_fd(arg); break;
	case FSL_IOCTL_BLKDEVPUT: ret = kfsl_rt_uninit(); break;
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

extern int kernbrowser_entry(int argc, char* argv[]);
extern int kernbrowser_exit(void);
static int __init init_fsl_mod(void)
{
	int	err;

	err = misc_register(&fsl_dev);
	if (err) {
		printk(KERN_ERR "fsl: misc dev register failed\n");
		goto done;
	}
	printk("fsl: Hello.\n");
	err = kernbrowser_entry(0, NULL);
	if (err) {
		printk(KERN_ERR "fsl: could not register fs");
	}
done:
	return err;
}

static void __exit exit_fsl_mod(void)
{
	int	err;

	kernbrowser_exit();
	kfsl_rt_uninit();
	err = misc_deregister(&fsl_dev);
	if (err)
		printk(KERN_ERR "fsl: failed to unregister misc dev\n");
	printk("fsl: Goodbye.\n");
}

module_init(init_fsl_mod)
module_exit(exit_fsl_mod)
