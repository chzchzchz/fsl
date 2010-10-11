#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "debug.h"
#include "runtime.h"

typedef uint64_t logaddr_t;
#define addr2log(x)	((x) >> 6)
#define log2addr(x)	((x) << 6)
//	addr >>= 3;	/* 3=bits */
//	addr >>= 3;	/* 3 => 8 byte units */


static void fsl_io_log(struct fsl_rt_io* io, uint64_t addr);

uint64_t __getLocal(
	const struct fsl_rt_closure* clo,
	uint64_t bit_off, uint64_t num_bits)
{
	uint8_t		buf[10];
	uint64_t	ret;
	int		i;
	size_t		br;

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

	if (fseeko(fsl_get_io()->io_backing, bit_off / 8, SEEK_SET) != 0) {
		fprintf(stderr, "BAD SEEK! bit_off=%"PRIx64"\n", bit_off);
		exit(-2);
	}

	if ((bit_off % 8) != 0) {
		fprintf(stderr,
			"NOT SUPPORTED: UNALIGNED ACCESS bit_off=%"PRIx64"\n",
			bit_off);
		exit(-3);
	}

	fsl_io_log(fsl_get_io(), bit_off);

	br = fread(buf, (num_bits + 7) / 8, 1, fsl_get_io()->io_backing);
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
	ret->io_accessed_idx = IO_IDX_STOPPED;

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

bool fsl_io_log_contains(struct fsl_rt_io* io, diskoff_t addr)
{
	int		i;
	logaddr_t 	logaddr;

	logaddr = addr2log(addr);
	for (i = 0; i < io->io_accessed_idx; i++) {
		if (io->io_accessed[i] == logaddr)
			return true;
	}

	return false;
}

void fsl_io_log_start(struct fsl_rt_io* io)
{
	assert (io->io_accessed_idx == IO_IDX_STOPPED);
	io->io_accessed_idx = 0;
}

void fsl_io_log_stop(struct fsl_rt_io* io)
{
	assert (io->io_accessed_idx != IO_IDX_STOPPED);
	io->io_accessed_idx = IO_IDX_STOPPED;
}

static void fsl_io_log(struct fsl_rt_io* io, diskoff_t addr)
{
	if (io->io_accessed_idx == IO_IDX_STOPPED) return;
	assert (io->io_accessed_idx < IO_MAX_ACCESS);

	/* round down to bytes */
	if (fsl_io_log_contains(io, addr)) return;
	io->io_accessed[io->io_accessed_idx++] = addr2log(addr);
}

void fsl_io_log_dump(struct fsl_rt_io* io, FILE* f)
{
	unsigned int	i;

	for (i = 0; i < io->io_accessed_idx; i++) {
		logaddr_t	logaddr;

		logaddr = io->io_accessed[i];
		fprintf(f, "%d. [%"PRIu64"--%"PRIu64"]\n",
			i,
			log2addr(logaddr),
			log2addr(logaddr+1)-1);
	}
}
