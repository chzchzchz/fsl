//#define DEBUG_IO
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "debug.h"
#include "runtime.h"
#include "cache.h"
#include "log.h"

uint64_t __getLocal(
	const struct fsl_rt_closure* clo,
	uint64_t bit_off, uint64_t num_bits)
{
	uint64_t		ret;
	struct fsl_rt_io	*io;

	assert (num_bits <= 64);
	assert (num_bits > 0);

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_ACCESS);
	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_PHYSACCESS);
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

		ret = fsl_io_cache_get(io, 8*(bit_off/8), 8);
		ret >>= bit_off % 8;
		if (bytes_full != 0) {
			uint64_t	v;
			v = fsl_io_cache_get(
				io, 8*(1+(bit_off/8)), 8*bytes_full);
			ret |= v << bits_left;
		}
	} else {
		/* common path */
		ret = fsl_io_cache_get(io, bit_off, num_bits);
	}
	if (io->io_cb_any != NULL) io->io_cb_any(io, bit_off);

	DEBUG_IO_WRITE(
		"Returning IO: bitoff = %"PRIu64" // bits=%"PRIu64" // v = %"PRIu64,
			bit_off, num_bits, ret);

	DEBUG_IO_LEAVE();

	return ret;
}

void fsl_io_write(uint64_t bit_off, uint64_t val, uint64_t num_bits)
{
	struct fsl_rt_io	*io;
	int			i;
	char			buf[8];
	size_t			bw;

	io = fsl_get_io();

	/* 1. write to file */
	/* 2. flush cache */
	if ((bit_off % 8)) {
		printf("ARGH");
		assert (0 == 1 && "UNALIGNED ACCESS");
	}
	if ((num_bits % 8)) {
		printf("WHRG");
		assert (0 == 1 && "NON-BYTE BITS");
	}

	for (i = 0; i < num_bits; i+=8) buf[i] = (val >> i) & 0xff;

	bw = fwrite(buf, num_bits/8, 1, io->io_backing);
	assert (bw > 0);

	assert (0 == 1 && "INVALIDATE CACHE-- SUCKA");
}


uint64_t __getLocalArray(
	const struct fsl_rt_closure* clo,
	uint64_t idx, uint64_t bits_in_type,
	uint64_t base_offset, uint64_t bits_in_array)
{
	diskoff_t	real_off, array_off;

	assert (bits_in_type <= 64);

	array_off = bits_in_type * idx;
	if (array_off > bits_in_array) {
		fprintf(stderr,
			"ARRAY OVERFLOW: idx=%"PRIu64
			",bit=%"PRIu64",bia=%"PRIu64"\n",
			idx, bits_in_type, bits_in_array);
		exit(-5);
	}

	real_off = base_offset + (bits_in_type * idx);
	return __getLocal(clo, real_off, bits_in_type);
}

void __writeVal(uint64_t loc, uint64_t sz, uint64_t val)
{
	/* assumes *physical address* */
	fsl_wlog_add(&fsl_get_io()->io_wlog, loc, val, sz);
}

struct fsl_rt_io* fsl_io_alloc(const char* backing_fname)
{
	struct fsl_rt_io	*ret;
	FILE			*f;

	assert (backing_fname != NULL);

	f = fopen(backing_fname, "r");
	if (f == NULL)
		return NULL;

	ret = malloc(sizeof(*ret));
	memset(ret, 0, sizeof(*ret));

	ret->io_backing = f;
	printf("%p\n", ret->io_backing);
	fsl_rlog_init(ret);
	fsl_wlog_init(&ret->io_wlog);
	fsl_io_cache_init(&ret->io_cache);

	printf("%p\n", ret->io_backing);
	return ret;
}

void fsl_io_free(struct fsl_rt_io* io)
{
	assert (io != NULL);
	fclose(io->io_backing);
	free(io);
}

ssize_t fsl_io_size(struct fsl_rt_io* io)
{
	fseeko(io->io_backing, 0, SEEK_END);
	__FROM_OS_BDEV_BYTES = ftello(io->io_backing);
	return __FROM_OS_BDEV_BYTES;
}

fsl_io_callback fsl_io_hook(
	struct fsl_rt_io* io, fsl_io_callback cb,
	int cb_type)
{
	fsl_io_callback	old_cb;

	assert (cb_type < IO_CB_NUM);
	old_cb = io->io_cb[cb_type];
	io->io_cb[cb_type] = cb;

	return old_cb;
}

void fsl_io_unhook(struct fsl_rt_io* io, int cb_type)
{
	assert (cb_type < IO_CB_NUM);
	assert (io->io_cb[cb_type] != NULL && "Unhooking NULL function");

	io->io_cb[cb_type] = NULL;
}

void fsl_io_steal_wlog(struct fsl_rt_io* io, struct fsl_rt_wlog* dst)
{
	fsl_wlog_copy(dst, &io->io_wlog);
	fsl_wlog_init(&io->io_wlog);
}

void fsl_io_do_wpkt(const struct fsl_rt_table_wpkt* wpkt, uint64_t* params)
{
	assert (0 == 1 && "STUB");
}

void fsl_io_read_bytes(void* buf, unsigned int byte_c, uint64_t off)
{
	struct fsl_rt_io	*io = fsl_get_io();
	size_t			br;

	if (fseeko(io->io_backing, off, SEEK_SET) != 0) {
		fprintf(stderr, "BAD SEEK! bit_off=%"PRIu64"\n", off);
		assert (0 == 1);
	}

	br = fread(buf, byte_c, 1, io->io_backing);
	assert (br > 0);
}

void fsl_io_write_bytes(void* buf, unsigned int byte_c, uint64_t off)
{
	struct fsl_rt_io	*io = fsl_get_io();
	size_t			bw;

	if (fseeko(io->io_backing, off, SEEK_SET) != 0) {
		fprintf(stderr, "BAD SEEK! bit_off=%"PRIu64"\n", off);
		assert (0 == 1);
	}

	bw = fwrite(buf, byte_c, 1, io->io_backing);
	assert (bw > 0);

	fsl_io_cache_drop_bytes(io, off, byte_c);
}
