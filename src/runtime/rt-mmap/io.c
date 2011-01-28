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
#include <byteswap.h>

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
		int	bits_left, bits_right;
		int	bytes_full;

		bits_left = 8 - (bit_off % 8);
		if (bits_left > num_bits) bits_left = num_bits;
		bytes_full = (num_bits - bits_left) / 8;
		bits_right = num_bits - (8*bytes_full + bits_left);
		assert (bits_left > 0 && bits_left < 8);
		assert (bits_right == 0);

		ret = get_nth_8b(io->io_priv->iop_mmap_ptr, bit_off/8);
		ret >>= bit_off % 8;
		if (bytes_full != 0) {
			uint64_t	v;
			v = get_nth_64b(io->io_priv->iop_mmap_ptr, (1+(bit_off/8)));
			v &= (1LL << (8*bytes_full))-1;
			ret |= v << bits_left;
		}
		ret &= (1LL << num_bits) - 1;
	} else {
		/* common path */
		ret = get_nth_64b(io->io_priv->iop_mmap_ptr, bit_off/8);
		if (num_bits < 64) ret &= ((1LL << (num_bits)) - 1);
	}

	if (io->io_cb_any != NULL) io->io_cb_any(io, bit_off);

	if (__fsl_mode == FSL_MODE_BIGENDIAN) {
	#ifdef FSL_LITTLE_ENDIAN
		switch(num_bits) {
		case	64:	ret = bswap_64(ret); break;
		case	32:	ret = bswap_32(ret);; break;
		case	16:	ret = bswap_16(ret); break;
		}
	#endif
	} else {
	#ifdef FSL_BIG_ENDIAN
		switch(num_bits) {
		case	64:	ret = bswap_64(ret); break;
		case	32:	ret = bswap_32(ret); break;
		case	16:	ret = bswap_16(ret); break;
		}
	#endif
	}

	return ret;
}

uint64_t __getLocal(
	const struct fsl_rt_closure* clo,
	uint64_t bit_off, uint64_t num_bits)
{
	uint64_t		ret;

	assert (num_bits <= 64);
	assert (num_bits > 0);

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_ACCESS);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_READ, num_bits);

	DEBUG_IO_ENTER();

	DEBUG_IO_WRITE("Requesting IO: bitoff=%"PRIu64, bit_off);

	if (clo->clo_xlate != NULL) {
		/* xlate path */
		uint64_t	bit_off_old, bit_off_next, bit_off_last;

		bit_off_old = bit_off;
		DEBUG_IO_WRITE("BIT_COUNT=%"PRIu64, num_bits);
		DEBUG_IO_WRITE("BIT_OFF_OLD=%"PRIu64, bit_off_old);
		bit_off = fsl_virt_xlate(clo, bit_off_old);

		/* ensure read will go to only a contiguous range (e.g. xlate
		 * not sliced too thin) */
		bit_off_next = bit_off_old + 8*((num_bits - 1)/8);
		DEBUG_IO_WRITE("BIT_OFF_NEXT=%"PRIu64, bit_off_next);
		bit_off_last = fsl_virt_xlate(clo, bit_off_next);
		assert ((bit_off + 8*((num_bits-1)/8)) == (bit_off_last) &&
			"Discontiguous getLocal not permitted");
	}

	ret = __getLocalPhys(bit_off, num_bits);

	DEBUG_IO_WRITE(
		"Returning IO: bitoff = %"PRIu64" // bits=%"PRIu64" // v = %"PRIu64,
			bit_off, num_bits, ret);

	DEBUG_IO_LEAVE();

	if (num_bits == 1) assert (ret < 2 && "BADMASK.");

	return ret;
}

struct fsl_rt_io* fsl_io_alloc(const char* backing_fname)
{
	struct fsl_rt_io	*ret;
	int			fd;
	struct stat		s;

	assert (backing_fname != NULL);

	if (stat(backing_fname, &s) == -1) return NULL;

	fd = open(backing_fname, O_LARGEFILE | O_RDWR /* O_RDONLY */);
	if (fd == -1) return NULL;

	ret = malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));

	ret->io_priv = malloc(sizeof(struct fsl_rt_io_priv));
	ret->io_priv->iop_fd = fd;
	ret->io_blksize = 4096;
	if (S_ISBLK(s.st_mode)) {
		if (ioctl(fd, BLKGETSIZE, &ret->io_blk_c) != 0) {
			free(ret);
			return NULL;
		}
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
