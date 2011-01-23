#ifndef FSL_MMAP_IO_H
#define FSL_MMAP_IO_H

struct fsl_rt_io_priv
{
	int			iop_fd;
	uint8_t			*iop_mmap_ptr;
};

#endif
