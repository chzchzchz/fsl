/* main runtime file */
//#define DEBUG_RT
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "debug.h"
#include "runtime.h"
#include "hits.h"

extern uint64_t fsl_num_types;

struct fsl_rt_ctx* 		fsl_env;
static int			cur_debug_write_val = 0;

static void fsl_vars_from_env(struct fsl_rt_ctx* fctx);

void __debugOutcall(uint64_t v)
{
	printf("DEBUG_WRITE: %"PRIu64"\n", v);
	cur_debug_write_val = v;
}


void __debugClosureOutcall(uint64_t typenum, struct fsl_rt_closure* clo)
{
	assert (clo != NULL);

	printf("DEBUG_WRITE_TYPE: %s-- voff=%"PRIu64". poff=%"PRIu64". xlate=%p\n",
		tt_by_num(typenum)->tt_name,
		clo->clo_offset,
		(clo->clo_xlate) ?
			fsl_virt_xlate(clo, clo->clo_offset) : clo->clo_offset,
		clo->clo_xlate);
}

/* TODO FSL_FAILED should have some unique number so we know why we failed */
uint64_t fsl_fail(void)
{
	fprintf(stderr, "Failed for some reason...\n");
	exit(-1);
}

/* compute the number of bits in a given array */
/* TODO: need way to check to see if uses an index value-- if not, we
 * don't need to reload the params all the time */
typesize_t __computeArrayBits(
	uint64_t typenum_parent,
	const struct fsl_rt_closure* clo_parent,
	unsigned int field_idx,
	uint64_t num_elems)
{
	const struct fsl_rtt_type	*parent_tt, *elem_tt;
	const struct fsl_rtt_field	*tf;
	typenum_t			elem_type;
	unsigned int			i;
	typesize_t			total_bits;

	DEBUG_RT_ENTER();

	assert (typenum_parent < fsl_rtt_entries && "Bad parent type");

	parent_tt = tt_by_num(typenum_parent);
	assert (field_idx < parent_tt->tt_field_c && "Fieldnum overflow");

	tf = &parent_tt->tt_field_table[field_idx];
	if (field_idx != tf->tf_fieldnum)
		printf("field_idx = %d, tf->tf_fieldnum=%d, tf->tf_fieldname=%s\n",
			field_idx,
			tf->tf_fieldnum, tf->tf_fieldname);
	assert (field_idx == tf->tf_fieldnum && "Fieldnum mismatch");

	elem_type = tf->tf_typenum;
	assert (elem_type < fsl_rtt_entries && "Bad array type");
	elem_tt = tt_by_num(elem_type);

	NEW_EMPTY_CLO			(cur_clo, elem_type);

	/* save old dyn closure value */
	DEBUG_RT_WRITE("Get initial closure for type '%s'", elem_tt->tt_name);

	/* get base closure */
	LOAD_CLO(&cur_clo, tf, clo_parent);

	total_bits = 0;
	DEBUG_RT_WRITE("Looping over %d elems of type %s", num_elems, elem_tt->tt_name);
	for (i = 0; i < num_elems; i++) {
		typesize_t		cur_size;

		cur_size = elem_tt->tt_size(&cur_clo);

		total_bits += cur_size;
		cur_clo.clo_offset += cur_size;

		/* don't overflow */
		if (i != (num_elems - 1))
			tf->tf_params(clo_parent, i+1, cur_clo.clo_params);
	}
	DEBUG_RT_WRITE("Looping finished. sz=%"PRIu64, total_bits);


	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_COMPUTEARRAYBITS);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_COMPUTEARRAYBITS_LOOPS, num_elems);

	DEBUG_RT_LEAVE();

	return total_bits;
}

/* not exposed to llvm */
struct fsl_rt_ctx* fsl_rt_init(const char* fsl_rt_backing_fname)
{
	struct fsl_rt_ctx	*fsl_ctx;

	fsl_ctx = malloc(sizeof(struct fsl_rt_ctx));
	fsl_ctx->fctx_io = fsl_io_alloc(fsl_rt_backing_fname);
	fsl_ctx->fctx_num_types = fsl_num_types;
	memset(&fsl_ctx->fctx_stat, 0, sizeof(struct fsl_rt_stat));

	return fsl_ctx;
}

static const char* fsl_stat_fields[FSL_NUM_STATS] =
{
	"getLocals",		/* FSL_STAT_ACCESS */
	"getLocalsPhy", 	/* FSL_STAT_PHYSACCESS */
	"br",			/* FSL_STAT_BITS_READ */
	"xlate_call",		/* FSL_STAT_XLATE_CALL */
	"xlate_alloc",		/* FSL_STAT_XLATE_ALLOC */
	"computeArrayBits",	/* FSL_STAT_COMPUTEARRAYBITS */
	"dyn_set",		/* FSL_STAT_DYNSET */
	"get_param",		/* FSL_STAT_GETPARAM */
	"get_closure",		/* FSL_STAT_GETCLOSURE */
	"get_offset",		/* FSL_STAT_GETOFFSET */
	"dyn_copy",		/* FSL_STAT_DYNCOPY */
	"dyn_alloc",		/* FSL_STAT_DYNALLOC */
	"typeinfo_alloc",	/* FSL_STAT_TYPEINFO_ALLOC */
	"comparray_elems",	/* FSL_STAT_COMPUTEARRAYBITS_LOOPS */
	"writes",		/* FSL_STAT_WRITES */
	"bw",			/* FSL_STAT_BITS_WRITTEN */
	"xlate_hit",		/* FSL_STAT_XLATE_HIT */
	"iocache_hit",		/* FSL_STAT_IOCACHE_MISS */
	"iocache_miss"		/* FSL_STAT_IOCACHE_HIT */
};

static void fsl_rt_dump_stats(struct fsl_rt_ctx* fctx)
{
	const char	*stat_fname;
	FILE		*out_file;
	unsigned int	i;

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

	fprintf(out_file, "# dumping stats.\n");
	for (i = 0; i < FSL_NUM_STATS; i++) {
		fprintf(out_file,
			"{ '%s' : %"PRIu64" }\n",
			fsl_stat_fields[i],
			FSL_STATS_GET(&fctx->fctx_stat, i));
	}
	fprintf(out_file, "# stats done.\n");

	if (stat_fname != NULL)
		fclose(out_file);
}

void fsl_rt_uninit(struct fsl_rt_ctx* fctx)
{
	assert (fctx != NULL);

	fsl_rt_dump_stats(fctx);
	fsl_io_free(fctx->fctx_io);
	free(fctx);
}

static void fsl_vars_from_env(struct fsl_rt_ctx* fctx)
{
	assert (fctx != NULL);
	assert (fctx->fctx_io != NULL && "Is the file open?");

	__FROM_OS_BDEV_BYTES = fsl_io_size(fctx->fctx_io);
	__FROM_OS_BDEV_BLOCK_BYTES = fctx->fctx_io->io_blksize;
	__FROM_OS_SB_BLOCKSIZE_BYTES = fctx->fctx_io->io_blksize;
}

/* main entry point for tool executable --
 * set some stuff up and then run tool */
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
	/* track hits, if applicable */
	fsl_hits_init();

	tool_ret = tool_entry(argc-2, argv+2);

	fsl_hits_uninit();
	fsl_rt_uninit(fsl_env);

	return tool_ret;
}
