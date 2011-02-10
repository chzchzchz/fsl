#include <linux/module.h>

MODULE_LICENSE("BSD");
MODULE_AUTHOR("ANTONY ROMANO");

static int __init init_fsl_fs(void)
{
	int	err = 0;

	printk("HELLO WORLD\n");
//	err = register_filesystem(&fsl_fs_type);
//	if (err) goto done;

done:
	return err;
}

static void __exit exit_fsl_fs(void)
{
//	unregister_filesystem(&fsl_fs_type);
}

module_init(init_fsl_fs)
module_exit(exit_fsl_fs)
