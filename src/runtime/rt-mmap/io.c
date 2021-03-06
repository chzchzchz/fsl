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
#include <sys/mman.h>
#include <endian.h>

#include "debug.h"
#include "runtime.h"
#include "log.h"
#include "io.h"
#include "io_priv.h"

#define get_nth_64b(x,n)	*((uint64_t*)(&x[n]))
#define get_nth_8b(x,n)		*((uint8_t*)(&x[n]))

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
		ret = get_nth_64b(io->io_priv->iop_mmap_ptr, bit_off/8);
		if (num_bits < 64) ret &= ((1UL << (num_bits)) - 1);
		if (__fsl_mode == FSL_MODE_BIGENDIAN) {
		#ifdef FSL_LITTLE_ENDIAN
			ret = __swapBytes(ret, num_bits/8);
		#endif
		} else {
		#ifdef FSL_BIG_ENDIAN
			ret = __swapBytes(ret, num_bits/8);
		#endif
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

	fd = open(backing_fname, O_LARGEFILE | O_RDWR /* O_RDONLY */);
	if (fd == -1) return NULL;

	ret = malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));

	ret->io_priv = malloc(sizeof(struct fsl_rt_io_priv));
	if (ret->io_priv == NULL) goto bad_iopriv;

	ret->io_priv->iop_fd = fd;
	ret->io_blksize = 4096;
	if (S_ISBLK(s.st_mode)) {
		if (ioctl(fd, BLKGETSIZE, &ret->io_blk_c) != 0) goto bad_getblksize;
		ret->io_blk_c *= 512;
	} else {
		ret->io_blk_c = lseek64(ret->io_priv->iop_fd, 0, SEEK_END);
		lseek64(ret->io_priv->iop_fd, 0, SEEK_SET);
	}
	ret->io_blk_c /= ret->io_blksize;

	ret->io_priv->iop_mmap_ptr = mmap(
		NULL, ret->io_blk_c * 4096,
		PROT_READ | PROT_WRITE, MAP_SHARED,
		ret->io_priv->iop_fd, 0);

	fsl_rlog_init(ret);
	fsl_wlog_init(&ret->io_wlog);

	return ret;

bad_getblksize:
	free(ret->io_priv);
bad_iopriv:
	close(fd);
	free(ret);
	return NULL;
}

void fsl_io_free(struct fsl_rt_io* io)
{
	assert (io != NULL);
	munmap(io->io_priv->iop_mmap_ptr, io->io_blksize * io->io_blk_c);
	close(io->io_priv->iop_fd);
	free(io);
}

void fsl_io_read_bytes(void* buf, unsigned int byte_c, uint64_t off)
{
	struct fsl_rt_io	*io = fsl_get_io();
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_READ, byte_c*8);
	memcpy(buf, &io->io_priv->iop_mmap_ptr[off], byte_c);
}

static void fsl_io_write_bit(uint64_t bit_off, uint64_t val, uint64_t num_bits)
{
	struct fsl_rt_io	*io;
	uint64_t		out_v;
	int			bit_left;
	int			mask;

	assert (num_bits < 8);

	io = fsl_get_io();
	bit_left = bit_off % 8;
	out_v = get_nth_8b(io->io_priv->iop_mmap_ptr, bit_off/8);

	/* mask for RMW */
	mask = (1LL << num_bits) - 1;
	val = val & ((1LL << num_bits) - 1);
	/* stick it in */
	out_v &= ~(mask << bit_left);
	out_v |= val << bit_left;

	/* write it */
	io->io_priv->iop_mmap_ptr[bit_off/8] = out_v;
}

void fsl_io_write(uint64_t bit_off, uint64_t val, uint64_t num_bits)
{
	struct fsl_rt_io	*io = fsl_get_io();
	int			i;

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_WRITES);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_WRITTEN, num_bits);
	if (io->io_cb_write != NULL) io->io_cb_write(io, bit_off);

	/* XXX HACK for partial write */
	if (num_bits == 1) {
		fsl_io_write_bit(bit_off, val, num_bits);
		return;
	}

	if ((bit_off % 8)) {
		printf("ARGH");
		assert (0 == 1 && "UNALIGNED ACCESS");
	}
	if ((num_bits % 8)) {
		printf("WHRG");
		assert (0 == 1 && "NON-BYTE BITS");
	}

	for (i = 0; i < num_bits; i+=8)
		io->io_priv->iop_mmap_ptr[(bit_off/8) + i/8] = (val >> i)&0xff;
}

void fsl_io_write_bytes(void* buf, unsigned int byte_c, uint64_t off)
{
	struct fsl_rt_io	*io = fsl_get_io();

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_WRITES);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_WRITTEN, byte_c*8);
	if (io->io_cb_write != NULL) io->io_cb_write(io, off*8);
	memcpy(&io->io_priv->iop_mmap_ptr[off], buf, byte_c);
}
