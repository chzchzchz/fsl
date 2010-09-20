#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "runtime.h"

extern struct fsl_rt_ctx	*fsl_env;

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

	if (fseeko(fsl_env->fctx_backing, bit_off / 8, SEEK_SET) != 0) {
		fprintf(stderr, "BAD SEEK! bit_off=%"PRIx64"\n", bit_off);
		exit(-2);
	}

	if ((bit_off % 8) != 0) {
		fprintf(stderr,
			"NOT SUPPORTED: UNALIGNED ACCESS bit_off=%"PRIx64"\n",
			bit_off);
		exit(-3);
	}

	br = fread(buf, (num_bits + 7) / 8, 1, fsl_env->fctx_backing);
	if (br != 1) {
		fprintf(stderr, "BAD FREAD bit_off=%"PRIx64" br=%d. bits=%d\n",
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

	if (fsl_rt_debug & FSL_DEBUG_FL_GETLOCAL) {
		printf("getlocal: bitoff = %"PRIu64" // bits=%"PRIu64" // v = %"PRIu64"\n",
			bit_off, num_bits, ret);
	}

	return ret;
}

uint64_t __getLocalArray(
	const struct fsl_rt_closure* clo,
	uint64_t idx, uint64_t bits_in_type,
	uint64_t base_offset, uint64_t bits_in_array)
{
	uint64_t	real_off, array_off;

	assert (0 == 1 && "NEW: CLOSURE");
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
	assert (0 == 1 && "WTF???");
//	return __getLocal(real_off, bits_in_type);
	return ~0; /* fake it for now */
}