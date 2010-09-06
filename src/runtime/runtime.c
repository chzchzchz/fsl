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

static struct fsl_rt_ctx* 	env;
int				fsl_rt_debug = 0;

static void fsl_vars_from_env(struct fsl_rt_ctx* fctx);

uint64_t __getLocal(uint64_t bit_off, uint64_t num_bits)
{
	uint8_t		buf[10];
	uint64_t	ret;
	int		i;
	size_t		br;

	assert (num_bits <= 64);

	if (fseeko(env->fctx_backing, bit_off / 8, SEEK_SET) != 0) {
		fprintf(stderr, "BAD SEEK! bit_off=%"PRIx64"\n", bit_off);
		exit(-2);
	}

	if ((bit_off % 8) != 0) {
		fprintf(stderr,
			"NOT SUPPORTED: UNALIGNED ACCESS bit_off=%"PRIx64"\n",
			bit_off);
		exit(-3);
	}

	br = fread(buf, (num_bits + 7) / 8, 1, env->fctx_backing);
	if (br != 1) {
		fprintf(stderr, "BAD FREAD bit_off=%"PRIx64"\n", bit_off);
		exit(-4);
	}

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

uint64_t __getDynOffset(uint64_t type_num)
{
	assert (type_num < env->fctx_num_types);
	assert (env->fctx_type_offsets[type_num] != ~0);

	return env->fctx_type_offsets[type_num];
}

void __getDynParams(uint64_t typenum, parambuf_t params_out)
{
	struct fsl_rt_table_type	*tt;

	assert (typenum < env->fctx_num_types);
	assert (env->fctx_type_offsets[typenum] != ~0);

	tt = tt_by_num(typenum);
	if (tt->tt_param_c == 0) return;

	memcpy(params_out, env->fctx_type_params[typenum], tt->tt_param_c*8);
}

void __setDyn(uint64_t type_num, diskoff_t offset, parambuf_t params)
{
	struct fsl_rt_table_type	*tt;

	assert (type_num < env->fctx_num_types);
	env->fctx_type_offsets[type_num] = offset;

	tt = tt_by_num(type_num);
	memcpy(env->fctx_type_params[type_num], params, 8*tt->tt_param_c);
}

void fsl_rt_dump_dyn(void)
{
	unsigned int	i;

	assert (env != NULL);
	for (i = 0; i < env->fctx_num_types; i++) {
		printf("type %2d (%s): %"PRIu64"\n",
			i,
			tt_by_num(i)->tt_name,
			env->fctx_type_offsets[i]);
	}
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

uint64_t __max6(
	uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
	uint64_t a5)
{
	uint64_t m = __max5(a0,a1,a2,a3,a4);
	return (m > a5) ? m : a5;
}

uint64_t __max7(
	uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
	uint64_t a5, uint64_t a6)
{
	uint64_t m = __max6(a0,a1,a2,a3,a4,a5);
	return (m > a6) ? m : a6;
}

/* TODO FSL_FAILED should have some unique number so we know why we failed */
uint64_t fsl_fail(void)
{
	fprintf(stderr, "Failed for some reason...\n");
	exit(-1);
}

/* compute the number of bits in a given array */
/* TODO: need a way to handle parent type values if we're computing
 * some sort of parameterized type.. */
typesize_t __computeArrayBits(
	uint64_t elem_type,
	diskoff_t off,
	parambuf_t params,
	uint64_t num_elems)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;
	diskoff_t			cur_off;
	typesize_t			total_bits;

	assert (elem_type < fsl_rt_table_entries);

	total_bits = 0;
	cur_off = off;
	tt = tt_by_num(elem_type);
	for (i = 0; i < num_elems; i++) {
		typesize_t		cur_size;

		__setDyn(elem_type, cur_off, params);
		cur_size = tt->tt_size(cur_off, params);
		total_bits += cur_size;
		cur_off += cur_size;
	}

	return total_bits;
}

/* not exposed to llvm */
struct fsl_rt_ctx* fsl_rt_init(const char* fsl_rt_backing)
{
	FILE			*f;
	struct fsl_rt_ctx	*fsl_ctx;
	unsigned int		i;

	f = fopen(fsl_rt_backing, "r");
	if (f == NULL)
		return NULL;

	fsl_ctx = malloc(sizeof(struct fsl_rt_ctx));
	fsl_ctx->fctx_backing = f;
	fsl_ctx->fctx_num_types = fsl_num_types;
	fsl_ctx->fctx_type_offsets = malloc(sizeof(uint64_t) * fsl_num_types);
	memset(	fsl_ctx->fctx_type_offsets,
		0xff,
		sizeof(uint64_t) * fsl_num_types);

	fsl_ctx->fctx_type_params = malloc(sizeof(uint64_t*) * fsl_num_types);
	for (i = 0; i < fsl_num_types; i++) {
		struct fsl_rt_table_type	*tt;
		uint64_t			*param_ptr;

		tt = tt_by_num(i);
		if (tt->tt_param_c == 0) {
			param_ptr = NULL;
		} else {
			unsigned int	param_byte_c;
			param_byte_c = sizeof(uint64_t) * tt->tt_param_c;
			param_ptr = malloc(param_byte_c);
			memset(param_ptr, 0xff, param_byte_c);
		}

		fsl_ctx->fctx_type_params[i] = param_ptr;
	}

	return fsl_ctx;
}

void fsl_rt_uninit(struct fsl_rt_ctx* fctx)
{
	unsigned int	i;

	assert (fctx != NULL);
	fclose(fctx->fctx_backing);
	free(fctx->fctx_type_offsets);

	for (i = 0; i < fsl_num_types; i++)
		free(fctx->fctx_type_params[i]);
	free(fctx->fctx_type_params);

	free(fctx);
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s filename\n", argv[0]);
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
	assert (fctx->fctx_backing != NULL);

	fseeko(fctx->fctx_backing, 0, SEEK_END);
	__FROM_OS_BDEV_BYTES = ftello(fctx->fctx_backing);
	__FROM_OS_BDEV_BLOCK_BYTES = 512;
	__FROM_OS_SB_BLOCKSIZE_BYTES = 512;
}
