#include <linux/pagemap.h>
#include <linux/buffer_head.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/types.h>
#include "io.h"
#include "iopriv.h"
#include "log.h"
#include "alloc.h"
#include "runtime.h"
#include "cache.h"
#include "debug.h"

struct fsl_rt_io* fsl_io_alloc_fd(unsigned long fd)
{
	struct fsl_rt_io	*ret;
	struct file		*file;
	struct inode		*ino;

	file = fget(fd);
	if (file == NULL) return NULL;

	ino = file->f_mapping->host;

	/* TODO: !S_ISREG(inode->i_mode) && */
	if (!S_ISBLK(ino->i_mode)) goto error;

	ret = fsl_alloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));
	if (ret == NULL) goto error_ret;

	ret->io_priv = fsl_alloc(sizeof(struct fsl_rt_io_priv));
	if (ret->io_priv == NULL) goto error_priv;
	ret->io_priv->iop_file = file;
	ret->io_priv->iop_ino = ino;
	ret->io_priv->iop_bdev = ino->i_bdev; /* bdgrab if not ino */

	ret->io_blksize = ino->i_bdev->bd_block_size;
	ret->io_blk_c = ino->i_size / ret->io_blksize;

	fsl_io_cache_init(&ret->io_priv->iop_cache);
	fsl_rlog_init(ret);
	fsl_wlog_init(&ret->io_wlog);

	return ret;

error_priv:
	fsl_free(ret);
error_ret:
error:
	fput(file);
	return NULL;
}

/* ownership of bd is managed outside of rt_io, but is exclusive */
/* (this is mainly to work with the sb/bdev model) */
struct fsl_rt_io* fsl_io_alloc_bdev(struct block_device* bd)
{
	struct fsl_rt_io	*ret;

	if (bd == NULL) goto error;

	ret = fsl_alloc(sizeof(*ret));
	if (ret == NULL) goto error_ret;
	memset(ret, 0, sizeof(*ret));

	ret->io_priv = fsl_alloc(sizeof(struct fsl_rt_io_priv));
	if (ret->io_priv == NULL) goto error_priv;
	ret->io_priv->iop_file = NULL;
	ret->io_priv->iop_ino = NULL;
	ret->io_priv->iop_bdev = bd;

	ret->io_blksize = bd->bd_block_size;
	ret->io_blk_c = bd->bd_inode->i_size / ret->io_blksize;

	fsl_io_cache_init(&ret->io_priv->iop_cache);
	fsl_rlog_init(ret);
	fsl_wlog_init(&ret->io_wlog);

	return ret;

error_priv:
	fsl_free(ret);
error:
error_ret:
	return NULL;
}

void fsl_io_free(struct fsl_rt_io* io)
{
	if (io->io_priv->iop_file != NULL) fput(io->io_priv->iop_file);
	fsl_free(io->io_priv);
	fsl_free(io);
}

uint64_t __getLocalPhys(uint64_t bit_off, uint64_t num_bits)
{
	struct fsl_rt_io	*io;
	uint64_t 		ret;

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_PHYSACCESS);

	io = fsl_get_io();
	if ((bit_off % 8) != 0) {
		if (__fsl_mode == FSL_MODE_BIGENDIAN) {
		#ifdef FSL_LITTLE_ENDIAN
			ret =  __getLocalPhysMisalignSwp(bit_off, num_bits);
		#else
			/* match: big/big */
			ret =  __getLocalPhysMisalign(bit_off, num_bits);
		#endif
		} else {
		#ifdef FSL_BIG_ENDIAN
			ret =  __getLocalPhysMisalignSwp(bit_off, num_bits);
		#else
			/* match: LITTLE/LITTLE */
			ret =  __getLocalPhysMisalign(bit_off, num_bits);
		#endif
		}
	} else {
		ret = fsl_io_cache_get(io, bit_off, num_bits);
		if ((num_bits % 8) == 0) {
			if (__fsl_mode == FSL_MODE_BIGENDIAN) {
			#ifdef FSL_LITTLE_ENDIAN
				ret = __swapBytes(ret, num_bits/8);
			#endif
			} else {
			#ifdef FSL_BIG_ENDIAN
				ret = __swapBytes(ret, num_bits/8);
			#endif
			}
		}
	}

	return ret;
}

static void fsl_io_write_bit(uint64_t bit_off, uint64_t val, uint64_t num_bits)
{
	struct fsl_rt_io	*io;
	uint64_t		out_v;
	int			bit_left, mask;
	struct buffer_head	*bh;

	FSL_ASSERT (num_bits < 8);

	io = fsl_get_io();
	bit_left = bit_off % 8;
	out_v = fsl_io_cache_get(io, bit_off - bit_left, 8);
	/* mask for RMW */
	mask = (1 << num_bits) - 1;
	val = val & ((1 << num_bits) - 1);
	/* stick it in */
	out_v &= ~(mask << bit_left);
	out_v |= val << bit_left;

	bh = __getblk(io_bdev(io),  io_boff_to_blk(io, bit_off/8), PAGE_SIZE);
	FSL_ASSERT (bh != NULL);

	lock_buffer(bh);
	bh_submit_read(bh);
	((char*)bh->b_data)[io_boff_to_blkoff(io,bit_off/8)] = out_v;
	set_buffer_uptodate(bh);
	unlock_buffer(bh);
	mark_buffer_dirty_inode(bh, io_inode(io));
	__brelse(bh);

	/* forget the cache XXX -- update cache instead of drop */
	fsl_io_cache_drop_bytes(io, bit_off / 8, 1);
}

void fsl_io_write(uint64_t bit_off, uint64_t val, uint64_t num_bits)
{
	struct fsl_rt_io	*io = fsl_get_io();
	struct buffer_head	*bh;
	int			i;

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_WRITES);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_WRITTEN, num_bits);
	if (io->io_cb_write != NULL) io->io_cb_write(io, bit_off);

	/* XXX HACK for partial write */
	if (num_bits == 1) {
		fsl_io_write_bit(bit_off, val, num_bits);
		goto done;
	}

	/* 1. write to file */
	/* 2. flush cache */
	FSL_ASSERT ( (bit_off % 8) == 0 && "UNALIGNED ACCESS");
	FSL_ASSERT ( (num_bits % 8) == 0 && "NON-BYTE BITCOUNT");
	FSL_ASSERT ( ((bit_off/8)/PAGE_SIZE) == ((bit_off+num_bits-1)/8)/PAGE_SIZE);

	bh = __getblk(io_bdev(io),  io_boff_to_blk(io, bit_off/8), PAGE_SIZE);
	FSL_ASSERT (bh != NULL);
	lock_buffer(bh);
	bh_submit_read(bh);
	for (i = 0; i < num_bits; i+=8) {
		char	*p = &((char*)bh->b_data)[
				io_boff_to_blkoff(io,bit_off/8)+(i/8)];
		*p = (val >> i) & 0xff;
	}
	set_buffer_uptodate(bh);
	unlock_buffer(bh);
	mark_buffer_dirty_inode(bh, io_inode(io));
	__brelse(bh);

done:
	fsl_io_cache_drop_bytes(io, bit_off/8, (num_bits+7)/8);
}

void fsl_io_read_bytes(void* buf, unsigned int byte_c, uint64_t off)
{
	struct fsl_rt_io	*io = fsl_get_io();
	struct buffer_head	*bh;
	size_t			br;
	uint64_t		cur_off;

	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_READ, byte_c*8);

	br = 0;
	cur_off = off;
	do {
		int		to_read;

		to_read = byte_c - br;
		if (to_read > PAGE_SIZE) to_read = PAGE_SIZE;

		bh = __getblk(io_bdev(io), io_boff_to_blk(io, cur_off), PAGE_SIZE);
		FSL_ASSERT (bh != NULL);
		lock_buffer(bh);
		bh_submit_read(bh);
		memcpy(	bh->b_data + io_boff_to_blkoff(io, cur_off),
			buf + br,
			to_read);
		unlock_buffer(bh);
		__brelse(bh);

		br += to_read;
	} while (br < byte_c);
}

void fsl_io_write_bytes(void* buf, unsigned int byte_c, uint64_t off)
{
	struct fsl_rt_io	*io = fsl_get_io();
	struct buffer_head	*bh;
	ssize_t			bw;
	uint64_t		cur_off;

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_WRITES);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_WRITTEN, byte_c*8);
	if (io->io_cb_write != NULL) io->io_cb_write(io, off*8);

	cur_off = off;
	bw = 0;
	do {
		int	to_write;

		to_write = byte_c - bw;
		to_write = (to_write > PAGE_SIZE) ? PAGE_SIZE : to_write;

		bh = __getblk(io_bdev(io), io_boff_to_blk(io, cur_off), PAGE_SIZE);
		FSL_ASSERT (bh != NULL);
		lock_buffer(bh);
		bh_submit_read(bh);
		memcpy(	bh->b_data + io_boff_to_blkoff(io,cur_off),
			buf + bw,
			to_write);
		set_buffer_uptodate(bh);
		unlock_buffer(bh);
		mark_buffer_dirty_inode(bh, io_inode(io));
		__brelse(bh);

		bw += to_write;
		cur_off += to_write;
	} while (bw < byte_c);

	fsl_io_cache_drop_bytes(io, off, byte_c);
}
