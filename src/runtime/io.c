//#define DEBUG_IO
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "debug.h"
#include "runtime.h"

static void	fsl_io_cache_init(struct fsl_io_cache* ioc);
static uint64_t	fsl_io_cache_get(struct fsl_rt_io* io, uint64_t bit_off, int num_bits);

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

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_ACCESS);
	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_PHYSACCESS);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_READ, num_bits);

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

	if ((bit_off % 8) != 0) {
		fprintf(stderr,
			"NOT SUPPORTED: UNALIGNED ACCESS bit_off=%"PRIx64"\n",
			bit_off);
		exit(-3);
	}

	/* common path */
	io = fsl_get_io();
	ret = fsl_io_cache_get(io, bit_off, num_bits);

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

static void fsl_io_cache_init(struct fsl_io_cache* ioc)
{
	unsigned int	i;

	assert (ioc != NULL);

	memset(ioc, 0, sizeof(struct fsl_io_cache));
	for (i = 0; i < FSL_IO_CACHE_ENTS; i++) {
		ioc->ioc_ents[i].ce_addr = ~0;
	}
}


static const uint8_t* fsl_io_cache_find(struct fsl_rt_io* io, uint64_t cache_line)
{
	struct fsl_io_cache	*ioc = &io->io_cache;
	char			*found_line;
	unsigned int		i;

	assert (cache_line != ~0);

	for (i = 0; i < FSL_IO_CACHE_ENTS; i++) {
		if (ioc->ioc_ents[i].ce_addr == cache_line) {
			ioc->ioc_hits++;
			return ioc->ioc_ents[i].ce_data;
		}
	}

	ioc->ioc_misses++;
	return NULL;
}

static struct fsl_io_cache_ent* fsl_io_cache_evict(struct fsl_io_cache* ioc)
{
	struct fsl_io_cache_ent	*to_evict;

	to_evict = &ioc->ioc_ents[ioc->ioc_misses % FSL_IO_CACHE_ENTS];
	/* Don't clear-- assume will be used immediately */

	return to_evict;
}

static const uint8_t* fsl_io_cache_put(struct fsl_rt_io* io, uint64_t cache_line)
{
	struct fsl_io_cache_ent		*ce;
	off_t				file_offset;
	size_t				br;

	/* get fresh line */
	ce = fsl_io_cache_evict(&io->io_cache);
	ce->ce_addr = cache_line;
	assert (ce != NULL);

	file_offset = cache_line * FSL_IO_CACHE_BYTES;
	if (fseeko(io->io_backing, file_offset, SEEK_SET) != 0) {
		fprintf(stderr, "BAD SEEK! bit_off=%"PRIx64"\n", file_offset*8);
		exit(-2);
	}

	br = fread(ce->ce_data, FSL_IO_CACHE_BYTES, 1, io->io_backing);
	if (br != 1) {
		fprintf(stderr, "BAD FREAD bit_off=%"PRIx64
				" br=%"PRIu64". bits=%"PRIu64"\n",
			file_offset*8, br, (uint64_t)FSL_IO_CACHE_BITS);
		exit(-4);
	}

	return ce->ce_data;
}

static uint64_t fsl_io_cache_get(struct fsl_rt_io* io, uint64_t bit_off, int num_bits)
{
	const uint8_t	*cache_line;
	uint64_t	ret;
	uint64_t	line_begin, line_end;
	int		i;

	DEBUG_IO_ENTER();

	line_begin = bit_off / FSL_IO_CACHE_BITS;
	line_end = ((bit_off + num_bits - 1) / FSL_IO_CACHE_BITS);

	DEBUG_IO_WRITE("LINE: %"PRIu64"--%"PRIu64"\n", line_begin, line_end);
	assert (line_begin == line_end && "OOPS! NOT CACHE ALIGNED");

	cache_line = fsl_io_cache_find(io, line_begin);
	if (cache_line == NULL) {
		DEBUG_IO_WRITE("Missed line: %"PRIu64"\n", line_begin);
		cache_line = fsl_io_cache_put(io, line_begin);
		if (io->io_cb != NULL) io->io_cb(io, bit_off);
	} else {
		DEBUG_IO_WRITE("Found line: %"PRIu64"\n", line_begin);
	}
	assert (cache_line != NULL);

	cache_line += (bit_off/8) % FSL_IO_CACHE_BYTES;
	assert ((bit_off % 8) == 0);

	// copy bits, byte by byte
	// XXX need to change to support arbitrary endians (this is little)
	ret = 0;
	for (i = (num_bits / 8) - 1; i >= 0; i--) {
		ret <<= 8;
		ret += cache_line[i];
	}

	DEBUG_IO_LEAVE();

	return ret;
}
