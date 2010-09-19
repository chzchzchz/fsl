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
#define env_get_dyn_clo(x)	(&(env->fctx_dyn_closures[(x)]))

extern uint64_t fsl_num_types;

static struct fsl_rt_ctx* 	env;
int				fsl_rt_debug = 0;
static int			cur_debug_write_val = 0;

static void fsl_vars_from_env(struct fsl_rt_ctx* fctx);

void __debugOutcall(uint64_t v)
{
	printf("DEBUG_WRITE: %"PRIu64"\n", v);
	cur_debug_write_val = v;
}

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

	env->fctx_stat.s_access_c++;
	env->fctx_stat.s_bits_read += num_bits;

	if (clo->clo_xlate != NULL) {
		uint64_t	bit_off_old, bit_off_last;

		bit_off_old = bit_off;
		bit_off = fsl_virt_xlate(clo, bit_off_old);
		bit_off_last = fsl_virt_xlate(clo, bit_off_old + (num_bits - 1));
		assert ((bit_off + (num_bits-1)) == (bit_off_last) &&
			"Discontiguous getLocal not permitted");
	}

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
		fprintf(stderr, "BAD FREAD bit_off=%"PRIx64" br=%d. bits=%d\n",
			bit_off, br, num_bits);
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

	if (cur_debug_write_val == 1111)
		printf("GOT: byte_off=%"PRIu64" / val=%"PRIu64" / bits=%d (v=%p)\n", bit_off/8, ret, num_bits, clo->clo_xlate);
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

uint64_t __getDynOffset(uint64_t type_num)
{
	const struct fsl_rt_closure	*src_clo;

	assert (type_num < env->fctx_num_types);

	src_clo = env_get_dyn_clo(type_num);
	assert (src_clo->clo_offset != ~0);

	return src_clo->clo_offset;
}

void __getDynParams(uint64_t typenum, parambuf_t params_out)
{
	struct fsl_rt_table_type	*tt;

	assert (typenum < env->fctx_num_types);

	tt = tt_by_num(typenum);
	if (tt->tt_param_c == 0) return;

	memcpy(	params_out,
		env_get_dyn_clo(typenum)->clo_params,
		sizeof(uint64_t)*tt->tt_param_c);
}

void __getDynClosure(uint64_t typenum, struct fsl_rt_closure* clo)
{
	const struct fsl_rt_closure*	src_clo;

	assert (typenum < env->fctx_num_types);
	assert (clo != NULL);

	src_clo = env_get_dyn_clo(typenum);
	clo->clo_offset = src_clo->clo_offset;
	memcpy(	clo->clo_params,
		src_clo->clo_params,
		sizeof(uint64_t)*tt_by_num(typenum)->tt_param_c);
	clo->clo_xlate = src_clo->clo_xlate;
}

void __setDyn(uint64_t type_num, const struct fsl_rt_closure* clo)
{
	struct fsl_rt_table_type	*tt;
	struct fsl_rt_closure		*dst_clo;

	assert (type_num < env->fctx_num_types);
	tt = tt_by_num(type_num);

	dst_clo = env_get_dyn_clo(type_num);
	dst_clo->clo_offset = clo->clo_offset;
	memcpy(dst_clo->clo_params, clo->clo_params, 8*tt->tt_param_c);
	dst_clo->clo_xlate = clo->clo_xlate;
}

void fsl_rt_dump_dyn(void)
{
	unsigned int	i;

	assert (env != NULL);
	for (i = 0; i < env->fctx_num_types; i++) {
		printf("type %2d (%s): %"PRIu64"\n",
			i,
			tt_by_num(i)->tt_name,
			env_get_dyn_clo(i)->clo_offset);
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
	fsl_ctx->fctx_dyn_closures = malloc(
		sizeof(struct fsl_rt_closure)*fsl_num_types);

	/* initialize all dynamic closures */
	for (i = 0; i < fsl_num_types; i++) {
		struct fsl_rt_table_type	*tt;
		struct fsl_rt_closure		*cur_clo;
		uint64_t			*param_ptr;

		cur_clo = &fsl_ctx->fctx_dyn_closures[i];
		tt = tt_by_num(i);
		if (tt->tt_param_c == 0) {
			param_ptr = NULL;
		} else {
			unsigned int	param_byte_c;
			param_byte_c = sizeof(uint64_t) * tt->tt_param_c;
			param_ptr = malloc(param_byte_c);
			memset(param_ptr, 0xff, param_byte_c);
		}

		cur_clo->clo_offset = ~0;	/* invalid offset*/
		cur_clo->clo_params = param_ptr;
		cur_clo->clo_xlate = NULL;
	}

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

	fprintf(out_file, "getLocals: %d\n", fctx->fctx_stat.s_access_c);
	fprintf(out_file, "br: %"PRIu64"\n", fctx->fctx_stat.s_bits_read / 8);

	if (stat_fname != NULL)
		fclose(out_file);
}

void fsl_rt_uninit(struct fsl_rt_ctx* fctx)
{
	unsigned int	i;

	assert (fctx != NULL);

	fsl_rt_dump_stats(fctx);

	fclose(fctx->fctx_backing);

	for (i = 0; i < fsl_num_types; i++) {
		struct fsl_rt_closure*	clo;
		clo = &fctx->fctx_dyn_closures[i];
		if (clo->clo_params != NULL)
			free(clo->clo_params);
	}
	free(fctx->fctx_dyn_closures);

	free(fctx);
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

int main(int argc, char* argv[])
{
	int	tool_ret;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s filename [tool opts]\n", argv[0]);
		return -1;
	}

	env = fsl_rt_init(argv[1]);
	if (env == NULL) {
		fprintf(stderr, "Could not initialize environment.\n");
		return -1;
	}

	fsl_vars_from_env(env);

	tool_ret = tool_entry(argc-2, argv+2);

	fsl_rt_uninit(env);

	return tool_ret;
}
