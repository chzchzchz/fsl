#ifndef FSL_PREAD_IO_H
#define FSL_PREAD_IO_H

#include "cache.h"

struct fsl_rt_io_priv
{
	int			iop_fd;
	struct fsl_io_cache	iop_cache;
};

#endif
