//#define DEBUG_IO
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <endian.h>

#include "debug.h"
#include "runtime.h"
#include "cache.h"
#include "log.h"
#include "io.h"
#include "io_priv.h"

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
		/* common path */
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

		if (io->io_cb_any != NULL) io->io_cb_any(io, bit_off);
	}

	return ret;
}

struct fsl_rt_io* fsl_io_alloc(const char* backing_fname)
{
	struct fsl_rt_io	*ret;
	int			fd, flags;
	struct stat		s;

	assert (backing_fname != NULL);

	if (stat(backing_fname, &s) == -1) return NULL;

	flags = O_LARGEFILE;
	flags |= (getenv("FSL_READONLY") != NULL) ? O_RDONLY : O_RDWR;

	fd = open(backing_fname, flags);
	if (fd == -1) return NULL;

	ret = malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));

	ret->io_priv = malloc(sizeof(struct fsl_rt_io_priv));

	ret->io_priv->iop_fd = fd;
	ret->io_blksize = s.st_blksize;
	if (S_ISBLK(s.st_mode)) {
		if (ioctl(fd, BLKGETSIZE, &ret->io_blk_c) != 0)
			goto error_bad_blkgetsz;
		ret->io_blk_c *= 512;
	} else {
		ret->io_blk_c = lseek64(ret->io_priv->iop_fd, 0, SEEK_END);
	}
	if (ret->io_blksize == 0) goto error_bad_blksz;

	ret->io_blk_c /= ret->io_blksize;

	fsl_rlog_init(ret);
	fsl_wlog_init(&ret->io_wlog);
	fsl_io_cache_init(&ret->io_priv->iop_cache);

	return ret;
error_bad_blkgetsz:
error_bad_blksz:
	close(fd);
	free(ret->io_priv);
	free(ret);
	return NULL;
}

void fsl_io_free(struct fsl_rt_io* io)
{
	assert (io != NULL);
	fsl_io_cache_uninit(&io->io_priv->iop_cache);
	close(io->io_priv->iop_fd);
	free(io->io_priv);
	free(io);
}

void fsl_io_read_bytes(void* buf, unsigned int byte_c, uint64_t off)
{
	struct fsl_rt_io	*io = fsl_get_io();
	size_t			br;

	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_READ, byte_c*8);
	br = pread64(io->io_priv->iop_fd, buf, byte_c, off);
	if (br < byte_c) {
		fprintf(stderr, "BAD READ! bit_off=%"PRIu64"\n", off);
		assert (0 == 1);
	}
}

static void fsl_io_write_bit(uint64_t bit_off, uint64_t val, uint64_t num_bits)
{
	struct fsl_rt_io	*io;
	uint64_t		out_v;
	int			bit_left;
	int			mask;
	ssize_t			bw;

	assert (num_bits < 8);

	io = fsl_get_io();
	bit_left = bit_off % 8;
	out_v = fsl_io_cache_get(io, bit_off - bit_left, 8);
	/* mask for RMW */
	mask = (1 << num_bits) - 1;
	val = val & ((1 << num_bits) - 1);
	/* stick it in */
	out_v &= ~(mask << bit_left);
	out_v |= val << bit_left;

	/* write it */
	bw = pwrite64(io->io_priv->iop_fd, &out_v, 1, bit_off/8);
	if (bw != 1) {
		fprintf(stderr,
			"ERROR WRITING BYTE %"PRIu64"\n",
			bit_off/8);
		assert (0 == 1);
		exit(-3);
	}

	/* forget the cache XXX -- update cache instead of drop */
	fsl_io_cache_drop_bytes(io, bit_off / 8, 1);
}

void fsl_io_write(uint64_t bit_off, uint64_t val, uint64_t num_bits)
{
	struct fsl_rt_io	*io = fsl_get_io();
	int			i;
	char			buf[8];
	ssize_t			bw;
	int			out_byte_c;

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_WRITES);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_WRITTEN, num_bits);
	if (io->io_cb_write != NULL) io->io_cb_write(io, bit_off);

	/* XXX HACK for partial write */
	if (num_bits == 1) {
		fsl_io_write_bit(bit_off, val, num_bits);
		return;
	}

	/* 1. write to file */
	/* 2. flush cache */
	if ((bit_off % 8)) {
		printf("ARGH! UNALIGNED ACCESS %"PRIu64"\n", bit_off);
		assert (0 == 1 && "UNALIGNED ACCESS");
	}
	if ((num_bits % 8)) {
		fprintf(stderr, "WHRG! NON-BYTE BITS %"PRIu64"\n", num_bits);
		assert (0 == 1 && "NON-BYTE BITS");
	}

	for (i = 0; i < num_bits; i+=8) buf[i/8] = (val >> i) & 0xff;

	out_byte_c = num_bits/8;
	bw = pwrite64(io->io_priv->iop_fd, buf, out_byte_c, bit_off/8);
	if (bw != out_byte_c) {
		fprintf(stderr,
			" ERROR WRITING TO BYTE %"PRIu64"\n",
			bit_off/8);
		assert (0 == 1);
		exit(-2);
	}

	fsl_io_cache_drop_bytes(io, bit_off/8, 1+((num_bits+7)/8));
}

void fsl_io_write_bytes(void* buf, unsigned int byte_c, uint64_t off)
{
	struct fsl_rt_io	*io = fsl_get_io();
	ssize_t			bw;

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_WRITES);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_WRITTEN, byte_c*8);
	if (io->io_cb_write != NULL) io->io_cb_write(io, off*8);

	bw = pwrite64(io->io_priv->iop_fd, buf, byte_c, off);
	if (bw != byte_c) {
		fprintf(stderr, "BAD WRITE! bit_off=%"PRIu64"\n", off);
	}
	assert (bw == byte_c);

	fsl_io_cache_drop_bytes(io, off, byte_c);
}
