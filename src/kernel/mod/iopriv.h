#ifndef FSLIOPRIV_H
#define FSLIOPRIV_H

#include "cache.h"

#define io_mapping(x)	((x)->io_priv->iop_ino->i_mapping)
#define io_bdev(x)	((x)->io_priv->iop_bdev)
#define io_inode(x)	((x)->io_priv->iop_ino)
#define io_boff_to_pageoff(y)	((y) % PAGE_SIZE)
#define io_boff_to_page(y)	((y)/ PAGE_SIZE)
#define io_boff_to_blk(x,y)	((y) / (x)->io_blksize)
#define io_boff_to_blkoff(x,y)	((y) % (x)->io_blksize)
#define io_bitoff_to_page(y)	io_boff_to_page((y)/8)

struct fsl_rt_io_priv {
	struct file		*iop_file;	/* NULL if bdev alloced */
	struct inode		*iop_ino;
	struct block_device	*iop_bdev;	/* from file */
	struct fsl_io_cache	iop_cache;
};

extern struct fsl_rt_io* fsl_io_alloc_bdev(struct block_device* bd);

#endif
