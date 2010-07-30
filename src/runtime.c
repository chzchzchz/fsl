/**
 * main runtime file
 */
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "runtime.h"

#define DYN_INVALID_TYPE	(~((uint64_t)0))

extern uint64_t fsl_num_types;

static struct fsl_rt_ctx* env;

static void fsl_vars_from_env(struct fsl_rt_ctx* fctx);

/* exported variables.. */
uint64_t __FROM_OS_BDEV_BYTES;
uint64_t __FROM_OS_BDEV_BLOCK_BYTES;

uint64_t __getLocal(uint64_t bit_off, uint64_t num_bits)
{
	uint8_t		buf[10];
	uint64_t	ret;
	int		i;

	assert (num_bits <= 64);

	if (fseeko(env->fctx_backing, bit_off / 8, SEEK_SET) == -1) {
		fprintf(stderr, "BAD SEEK! bit_off=%"PRIx64"\n", bit_off);
		exit(-2);
	}

	if ((bit_off % 8) != 0) {
		fprintf(stderr, 
			"NOT SUPPORTED: UNALIGNED ACCESS bit_off=%"PRIx64"\n",
			bit_off);
		exit(-3);
	}
	
	if (fread(buf, (num_bits + 7) / 8, 1, env->fctx_backing) == -1) {
		fprintf(stderr, "BAD FREAD bit_off=%"PRIx64"\n", bit_off);
		exit(-4);
	}

	ret = 0;
	for (i = (num_bits / 8) - 1; i >= 0; i--) {
		ret <<= 8;
		ret += buf[i];
	}

	return ret;
}

uint64_t __getLocalArray(
	uint64_t idx, uint64_t bits_in_type, 
	uint64_t base_offset, uint64_t bits_in_array)
{
	uint64_t	real_off, array_off;

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
	return __getLocal(real_off, bits_in_type);
}

uint64_t __getDyn(uint64_t type_num)
{
	assert (type_num < env->fctx_num_types);

	return env->fctx_type_offsets[type_num];
}

void __setDyn(uint64_t type_num, uint64_t offset)
{
	assert (type_num < env->fctx_num_types);
	env->fctx_type_offsets[type_num] = offset;
}

uint64_t __max2(uint64_t a0, uint64_t a1)
{
	return (a0 > a1) ? a0 : a1;
}

uint64_t __max3(uint64_t a0, uint64_t a1, uint64_t a2)
{
	if (a0 >= a1 && a0 >= a2)
		return a0;
	if (a1 >= a0 && a1 >= a2)
		return a1;
	return a2;
}
		
uint64_t __max4(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3)
{
	if (a0 >= a1 && a0 >= a2 && a0 >= a3)
		return a0;
	if (a1 >= a0 && a1 >= a2 && a1 >= a3)
		return a1;
	if (a2 >= a0 && a2 >= a1 && a2 >= a3)
		return a1;
	return a3;
}

uint64_t __max5(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4)
{
	uint64_t m = __max4(a0,a1,a2,a3);
	return (m > a4) ? m : a4;
}

/* TODO FSL_FAILED should have some unique number so we know why we failed */
uint64_t fsl_fail(void)
{
	fprintf(stderr, "Failed for some reason...\n");
	exit(-1);
}

/* not exposed to llvm */
struct fsl_rt_ctx* fsl_rt_init(const char* fsl_rt_backing)
{
	FILE			*f;
	struct fsl_rt_ctx	*fsl_ctx;

	f = fopen(fsl_rt_backing, "r");
	if (f == NULL)
		return NULL;

	fsl_ctx = malloc(sizeof(struct fsl_rt_ctx));
	fsl_ctx->fctx_backing = f;
	fsl_ctx->fctx_num_types = fsl_num_types;
	fsl_ctx->fctx_type_offsets = malloc(sizeof(uint64_t) * fsl_num_types);
	memset(	&fsl_ctx->fctx_type_offsets,
		0xff,
		sizeof(uint64_t) * fsl_num_types);
	

	return fsl_ctx;
}

void fsl_rt_uninit(struct fsl_rt_ctx* fctx)
{
	assert (fctx != NULL);
	fclose(fctx->fctx_backing);
	free(fctx->fctx_type_offsets);
	free(fctx);
}

int main(int argc, char* argv[])
{
	if (argc != 1) {
		fprintf(stderr, "Usage: %s filename", argv[1]);
		return -1;
	}	

	env = fsl_rt_init(argv[1]);
	if (env == NULL) {
		fprintf(stderr, "Could not initialize environment.\n");
		return -1;
	}

	fsl_vars_from_env(env);

	tool_entry();

	fsl_rt_uninit(env);

	return 0;
}

static void fsl_vars_from_env(struct fsl_rt_ctx* fctx)
{
	assert (fctx != NULL);

	fseeko(fctx->fctx_backing, 0, SEEK_END);
	__FROM_OS_BDEV_BYTES = ftello(fctx->fctx_backing);
	__FROM_OS_BDEV_BLOCK_BYTES = 512;
}
