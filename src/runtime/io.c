//#define DEBUG_IO
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "debug.h"
#include "runtime.h"
#include "cache.h"

uint64_t __getLocal(
	const struct fsl_rt_closure* clo,
	uint64_t bit_off, uint64_t num_bits)
{
	uint64_t		ret;
	int			i;
	size_t			br;
	struct fsl_rt_io	*io;

	assert (num_bits <= 64);
	assert (num_bits > 0);
	assert ((num_bits % 8) == 0 && "NO BIT READS");

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

		bit_off_next = bit_off_old + 8*((num_bits - 1)/8);
		DEBUG_IO_WRITE("BIT_OFF_NEXT=%"PRIu64, bit_off_next);
		bit_off_last = fsl_virt_xlate(clo, bit_off_next);
		assert ((bit_off + 8*((num_bits-1)/8)) == (bit_off_last) &&
			"Discontiguous getLocal not permitted");
	}

	if ((bit_off % 8) != 0) {
		fprintf(stderr,
			"NOT SUPPORTED: UNALIGNED ACCESS bit_off=%"PRIx64"\n",
			bit_off);
		exit(-3);
	}

	/* common path */
	io = fsl_get_io();
	ret = fsl_io_cache_get(io, bit_off, num_bits);
	if (io->io_cb_any != NULL) io->io_cb_any(io, bit_off);

	DEBUG_IO_WRITE(
		"Returning IO: bitoff = %"PRIu64" // bits=%"PRIu64" // v = %"PRIu64,
			bit_off, num_bits, ret);

	DEBUG_IO_LEAVE();

	return ret;
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
	fsl_io_log_init(ret);
	fsl_io_cache_init(&ret->io_cache);

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
