//#define DEBUG_IO
#include <unistd.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "debug.h"
#include "runtime.h"
#include "cache.h"

static struct fsl_io_cache_ent* fsl_io_cache_evict(
	struct fsl_io_cache* ioc, uint64_t cache_line);
static const uint8_t* fsl_io_cache_put(
	struct fsl_rt_io* io, uint64_t cache_line);
const uint8_t *fsl_io_cache_hitmiss(struct fsl_rt_io* io, uint64_t line_begin);

void fsl_io_cache_init(struct fsl_io_cache* ioc)
{
	unsigned int	i;

	assert (ioc != NULL);

	memset(ioc, 0, sizeof(struct fsl_io_cache));
	for (i = 0; i < FSL_IO_CACHE_ENTS; i++)
		ioc->ioc_ents[i].ce_addr = ~0;
}

const uint8_t* fsl_io_cache_find(struct fsl_io_cache* ioc, uint64_t cache_line)
{
	struct fsl_io_cache_ent	*ce;

	ce = &ioc->ioc_ents[cache_line % FSL_IO_CACHE_ENTS];
	if (ce->ce_addr == cache_line) {
		ioc->ioc_hits++;
		return ce->ce_data;
	}

	ioc->ioc_misses++;
	return NULL;
}

static struct fsl_io_cache_ent* fsl_io_cache_evict(
	struct fsl_io_cache* ioc, uint64_t cache_line)
{
	struct fsl_io_cache_ent	*to_evict;

	//to_evict = &ioc->ioc_ents[ioc->ioc_misses % FSL_IO_CACHE_ENTS];
	to_evict = &ioc->ioc_ents[cache_line % FSL_IO_CACHE_ENTS];
	/* Don't clear-- assume will be used immediately */

	return to_evict;
}


static const uint8_t* fsl_io_cache_put(struct fsl_rt_io* io, uint64_t cache_line)
{
	struct fsl_io_cache_ent		*ce;
	off_t				file_offset;
	size_t				br;

	/* get fresh line */
	ce = fsl_io_cache_evict(&io->io_cache, cache_line);
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

uint64_t fsl_io_cache_get_unaligned(struct fsl_rt_io* io, uint64_t bit_off, int num_bits)
{
	const uint8_t		*cache_line[2];
	uint8_t			join_buf[8];
	uint64_t		ret;
	uint64_t		line_begin, line_end;
	unsigned int		cache0_off;
	unsigned int		cache0_len, cache1_len, buf_len;
	int			i;

	DEBUG_IO_ENTER();

	line_begin = bit_off / FSL_IO_CACHE_BITS;
	line_end = ((bit_off + num_bits - 1) / FSL_IO_CACHE_BITS);
	assert ((line_begin+1) == line_end && "Cache lines too small?");

	cache_line[0] = fsl_io_cache_hitmiss(io, bit_off);
	cache_line[1] = fsl_io_cache_hitmiss(io, bit_off + FSL_IO_CACHE_BITS);

	assert ((bit_off % 8) == 0);

	cache0_off = (bit_off / 8) - (line_begin*FSL_IO_CACHE_BYTES);
	cache0_len = FSL_IO_CACHE_BYTES - cache0_off;
	cache1_len = ((num_bits+7)/8) - cache0_len;
	memcpy(join_buf, &cache_line[0][cache0_off], cache0_len);
	memcpy(&join_buf[cache0_len], cache_line[1], cache1_len);
	buf_len = cache0_len + cache1_len;
	assert (buf_len <= 8);
	assert (((num_bits / 8)-1) < 8);

	// copy bits, byte by byte
	// XXX need to change to support arbitrary endians (this is little)
	ret = 0;
	for (i = (num_bits / 8) - 1; i >= 0; i--) {
		ret <<= 8;
		ret += join_buf[i];
	}

	DEBUG_IO_LEAVE();

	return ret;

}

const uint8_t *fsl_io_cache_hitmiss(struct fsl_rt_io* io, uint64_t bit_off)
{
	const uint8_t	*cache_line;
	uint64_t	line_begin;

	line_begin = bit_off / FSL_IO_CACHE_BITS;
	cache_line = fsl_io_cache_find(&io->io_cache, line_begin);
	if (cache_line == NULL) {
		DEBUG_IO_WRITE("Missed line: %"PRIu64"\n", line_begin);
		cache_line = fsl_io_cache_put(io, line_begin);
		if (io->io_cb_miss != NULL) io->io_cb_miss(io, bit_off);
	} else {
		if (io->io_cb_hit != NULL) io->io_cb_hit(io, bit_off);
	}

	assert (cache_line != NULL);

	return cache_line;
}

uint64_t fsl_io_cache_get(struct fsl_rt_io* io, uint64_t bit_off, int num_bits)
{
	const uint8_t		*cache_line;
	uint64_t		ret;
	uint64_t		line_begin, line_end;
	int			i;

	DEBUG_IO_ENTER();

	line_begin = bit_off / FSL_IO_CACHE_BITS;
	line_end = ((bit_off + num_bits - 1) / FSL_IO_CACHE_BITS);

	DEBUG_IO_WRITE("LINE: %"PRIu64"--%"PRIu64"\n", line_begin, line_end);
	if (line_begin != line_end)
		return fsl_io_cache_get_unaligned(io, bit_off, num_bits);

	cache_line = fsl_io_cache_hitmiss(io, bit_off);

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
