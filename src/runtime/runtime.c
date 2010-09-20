/* main runtime file */
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "runtime.h"

extern uint64_t fsl_num_types;

struct fsl_rt_ctx* 		fsl_env;
static int			cur_debug_write_val = 0;

static void fsl_vars_from_env(struct fsl_rt_ctx* fctx);

void __debugOutcall(uint64_t v)
{
	printf("DEBUG_WRITE: %"PRIu64"\n", v);
	cur_debug_write_val = v;
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
	struct fsl_rt_closure* clo,
	uint64_t num_elems)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;
	diskoff_t			cur_off;
	typesize_t			total_bits;
	NEW_EMPTY_CLO			(old_dyn, elem_type);

	assert (elem_type < fsl_rt_table_entries);
	assert (clo->clo_xlate == NULL);

	total_bits = 0;
	cur_off = clo->clo_offset;;
	tt = tt_by_num(elem_type);
	/* save old dyn closure value */
	__getDynClosure(elem_type, &old_dyn);
	for (i = 0; i < num_elems; i++) {
		typesize_t		cur_size;
		NEW_CLO			(new_clo, cur_off, clo->clo_params);

		__setDyn(elem_type, &new_clo);
		cur_size = tt->tt_size(&new_clo);
		total_bits += cur_size;
		cur_off += cur_size;
	}

	/* reset to original */
	__setDyn(elem_type, &old_dyn);

	fsl_env->fctx_stat.s_comp_array_bits_c++;

	return total_bits;
}

/* not exposed to llvm */
struct fsl_rt_ctx* fsl_rt_init(const char* fsl_rt_backing_fname)
{
	FILE			*f;
	struct fsl_rt_ctx	*fsl_ctx;

	fsl_ctx = malloc(sizeof(struct fsl_rt_ctx));
	fsl_ctx->fctx_io = fsl_io_alloc(fsl_rt_backing_fname);
	fsl_ctx->fctx_num_types = fsl_num_types;
	fsl_ctx->fctx_dyn_closures = fsl_dyn_alloc();

	memset(&fsl_ctx->fctx_stat, 0, sizeof(struct fsl_rt_stat));

	return fsl_ctx;
}

static void fsl_rt_dump_stats(struct fsl_rt_ctx* fctx)
{
	const char	*stat_fname;
	FILE		*out_file;

	stat_fname = getenv(FSL_ENV_VAR_STATFILE);
	if (stat_fname != NULL) {
		out_file = fopen(stat_fname, "w");
		if (out_file == NULL) {
			fprintf(stderr,
				"Could not open statfile: %s\n",
				stat_fname);
			out_file = stdout;
		}
	} else
		out_file = stdout;

	fprintf(out_file, "getLocals %d\n", fctx->fctx_stat.s_access_c);
	fprintf(out_file, "getLocalsPhys %d\n", fctx->fctx_stat.s_phys_access_c);
	fprintf(out_file, "br %"PRIu64"\n", fctx->fctx_stat.s_bits_read / 8);
	fprintf(out_file, "xlate_call %"PRIu64"\n", fctx->fctx_stat.s_xlate_call_c);
	fprintf(out_file, "xlate_alloc %"PRIu64"\n", fctx->fctx_stat.s_xlate_alloc_c);

	fprintf(out_file, "comp_array_bits %"PRIu64"\n",
		fctx->fctx_stat.s_comp_array_bits_c);
	fprintf(out_file, "dyn_set %"PRIu64"\n",
		fctx->fctx_stat.s_dyn_set_c);
	fprintf(out_file, "get_param %"PRIu64"\n",
		fctx->fctx_stat.s_get_param_c);
	fprintf(out_file, "get_closure %"PRIu64"\n",
		fctx->fctx_stat.s_get_closure_c);
	fprintf(out_file, "get_offset %"PRIu64"\n",
		fctx->fctx_stat.s_get_offset_c);

	if (stat_fname != NULL)
		fclose(out_file);
}

void fsl_rt_uninit(struct fsl_rt_ctx* fctx)
{
	assert (fctx != NULL);

	fsl_rt_dump_stats(fctx);

	fsl_io_free(fctx->fctx_io);
	fsl_dyn_free(fctx->fctx_dyn_closures);

	free(fctx);
}

static void fsl_vars_from_env(struct fsl_rt_ctx* fctx)
{
	assert (fctx != NULL);
	assert (fctx->fctx_io != NULL);

	__FROM_OS_BDEV_BYTES = fsl_io_size(fctx->fctx_io);
	__FROM_OS_BDEV_BLOCK_BYTES = 512;
	__FROM_OS_SB_BLOCKSIZE_BYTES = 512;
}

struct fsl_rt_closure* fsl_rt_dyn_swap(struct fsl_rt_closure* dyns)
{
	struct fsl_rt_closure	*ret;

	ret = fsl_env->fctx_dyn_closures;
	fsl_env->fctx_dyn_closures = dyns;

	return ret;
}

int main(int argc, char* argv[])
{
	int	tool_ret;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s filename [tool opts]\n", argv[0]);
		return -1;
	}

	fsl_env = fsl_rt_init(argv[1]);
	if (fsl_env == NULL) {
		fprintf(stderr, "Could not initialize environment.\n");
		return -1;
	}

	fsl_vars_from_env(fsl_env);

	tool_ret = tool_entry(argc-2, argv+2);

	fsl_rt_uninit(fsl_env);

	return tool_ret;
}
