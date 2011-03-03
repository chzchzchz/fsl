//#define DEBUG_RT
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/module.h>
#include "runtime.h"
#include "io.h"
#include "fsl_ioctl.h"
#include "alloc.h"
#include "rt.h"
#include "iopriv.h"
#include "debug.h"

static bool is_inited = false;
extern uint64_t fsl_num_types;

bool kfsl_rt_inited(void) { return is_inited; }

static int init_rt_io(struct fsl_rt_io* fsl_io)
{
	struct fsl_rt_ctx	*fsl_ctx;

	fsl_ctx = fsl_alloc(sizeof(struct fsl_rt_ctx));
	if (fsl_ctx == NULL) {
		fsl_io_free(fsl_io);
		return -ENOMEM;
	}
	memset(fsl_ctx, 0, sizeof(struct fsl_rt_ctx));

	fsl_ctx->fctx_io = fsl_io;
	fsl_ctx->fctx_num_types = fsl_num_types;
	fsl_vars_from_env(fsl_ctx);

	fsl_env = fsl_ctx;

	DEBUG_RT_WRITE("LOAD THE MEMO");
	fsl_load_memo();

	is_inited = true;
	return 0;
}

int kfsl_rt_init_bdev(struct block_device* bdev)
{
	struct fsl_rt_io	*fsl_io;

	DEBUG_RT_WRITE("DO NOT REINIT");
	if (is_inited) return -EBUSY;

	DEBUG_RT_WRITE("ALOLC BDEV");
	fsl_io = fsl_io_alloc_bdev(bdev);
	if (fsl_io == NULL) return -EBADF;
	DEBUG_RT_WRITE("INIT RT");
	return init_rt_io(fsl_io);
}

int kfsl_rt_init_fd(unsigned long fd)
{
	struct fsl_rt_io	*fsl_io;

	if (is_inited) return -EBUSY;

	fsl_io = fsl_io_alloc_fd(fd);
	if (fsl_io == NULL) return -EBADF;
	return init_rt_io(fsl_io);
}

int kfsl_rt_uninit(void)
{
	int	ret;
	DEBUG_RT_WRITE("TRY RT UNINIT.");
	if (is_inited == false) return -ENXIO;
	DEBUG_RT_WRITE("RT UNINIT.");
	ret = fsl_rt_uninit(fsl_env);
	if (ret != 0) return ret;
	is_inited = false;
	return 0;
}
