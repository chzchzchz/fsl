#include <unistd.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "debug.h"
#include "runtime.h"
#include "cache.h"

static struct fsl_io_cache_ent* fsl_io_cache_evict(struct fsl_io_cache* ioc);
static const uint8_t* fsl_io_cache_put(struct fsl_rt_io* io, uint64_t cache_line);

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
	assert (line_begin == line_end && "OOPS! NOT CACHE ALIGNED");

	cache_line = fsl_io_cache_find(&io->io_cache, line_begin);
	if (cache_line == NULL) {
		DEBUG_IO_WRITE("Missed line: %"PRIu64"\n", line_begin);
		cache_line = fsl_io_cache_put(io, line_begin);
		if (io->io_cb_miss != NULL) io->io_cb_miss(io, bit_off);
	} else {
		if (io->io_cb_hit != NULL) io->io_cb_hit(io, bit_off);
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
