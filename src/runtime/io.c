#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "debug.h"
#include "runtime.h"

uint64_t __getLocal(
	const struct fsl_rt_closure* clo,
	uint64_t bit_off, uint64_t num_bits)
{
	uint8_t			buf[10];
	uint64_t		ret;
	int			i;
	size_t			br;
	struct fsl_rt_io	*io;

	assert (num_bits <= 64);
	assert (num_bits > 0);

	fsl_env->fctx_stat.s_access_c++;
	fsl_env->fctx_stat.s_phys_access_c++;
	fsl_env->fctx_stat.s_bits_read += num_bits;

	DEBUG_IO_ENTER();

	DEBUG_IO_WRITE("Requesting IO: bitoff=%"PRIu64, bit_off);

	if (clo->clo_xlate != NULL) {
		/* xlate path */
		uint64_t	bit_off_old, bit_off_last;

		bit_off_old = bit_off;
		bit_off = fsl_virt_xlate(clo, bit_off_old);
		bit_off_last = fsl_virt_xlate(clo, bit_off_old + (num_bits - 1));
		assert ((bit_off + (num_bits-1)) == (bit_off_last) &&
			"Discontiguous getLocal not permitted");
	}

	/* common path */
	io = fsl_get_io();

	if (fseeko(io->io_backing, bit_off / 8, SEEK_SET) != 0) {
		fprintf(stderr, "BAD SEEK! bit_off=%"PRIx64"\n", bit_off);
		exit(-2);
	}

	if ((bit_off % 8) != 0) {
		fprintf(stderr,
			"NOT SUPPORTED: UNALIGNED ACCESS bit_off=%"PRIx64"\n",
			bit_off);
		exit(-3);
	}

	if (io->io_cb != NULL) io->io_cb(io, bit_off);

	br = fread(buf, (num_bits + 7) / 8, 1, io->io_backing);
	if (br != 1) {
		fprintf(stderr, "BAD FREAD bit_off=%"PRIx64
				" br=%"PRIu64". bits=%"PRIu64"\n",
			bit_off, br, num_bits);
		exit(-4);
	}

	// copy bits, byte by byte
	// XXX need to change to support arbitrary endians
	ret = 0;
	for (i = (num_bits / 8) - 1; i >= 0; i--) {
		ret <<= 8;
		ret += buf[i];
	}

	DEBUG_IO_WRITE(
		"Returning IO: bitoff = %"PRIu64" // bits=%"PRIu64" // v = %"PRIu64"\n",
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
	ret->io_backing = f;
	fsl_io_log_init(ret);

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

fsl_io_callback fsl_io_hook(struct fsl_rt_io* io, fsl_io_callback cb)
{
	fsl_io_callback	old_cb;

	old_cb = io->io_cb;
	io->io_cb = cb;

	return io->io_cb;
}

void fsl_io_unhook(struct fsl_rt_io* io)
{
	assert (io->io_cb != NULL);
	io->io_cb = NULL;
}
