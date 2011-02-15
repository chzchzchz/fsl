#define _GNU_SOURCE
#include <inttypes.h>
#include <stdlib.h>
#include "io.h"
#include "alloc.h"
#include "hits.h"
#include "runtime.h"
#include <string.h>
#include <stdio.h>

extern uint64_t fsl_num_types;
extern const char* fsl_stat_fields[FSL_NUM_STATS];

static void fsl_rt_dump_stats(struct fsl_rt_ctx* fctx)
{
	const char	*stat_fname;
	FILE		*out_file;
	unsigned int	i;

	stat_fname = getenv(FSL_ENV_VAR_STATFILE);
	if (stat_fname != NULL) {
		out_file = fopen(stat_fname, "w");
		if (out_file == NULL) {
			fprintf(stderr, "Could not open statfile: %s\n",
				stat_fname);
			out_file = stdout;
		}
	} else
		out_file = stdout;

	fprintf(out_file, "# dumping stats.\n");
	for (i = 0; i < FSL_NUM_STATS; i++) {
		fprintf(out_file, "{ '%s' : %"PRIu64" }\n",
			fsl_stat_fields[i],
			FSL_STATS_GET(&fctx->fctx_stat, i));
	}
	fprintf(out_file, "# stats done.\n");

	if (stat_fname != NULL) fclose(out_file);
}

static struct fsl_rt_ctx* fsl_rt_init(const char* fsl_rt_backing_fname)
{
	struct fsl_rt_ctx	*fsl_ctx;

	fsl_ctx = fsl_alloc(sizeof(struct fsl_rt_ctx));
	if (fsl_ctx == NULL) return NULL;
	memset(fsl_ctx, 0, sizeof(*fsl_ctx));

	fsl_ctx->fctx_io = fsl_io_alloc(fsl_rt_backing_fname);
	fsl_ctx->fctx_num_types = fsl_num_types;

	fsl_vars_from_env(fsl_ctx);

	return fsl_ctx;
}

/* main entry point for tool executable --
 * set some stuff up and then run tool */
int main(int argc, char* argv[])
{
	int		tool_ret;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s filename [tool opts]\n", argv[0]);
		return -1;
	}

	fsl_env = fsl_rt_init(argv[1]);
	if (fsl_env == NULL) {
		fprintf(stderr, "Could not initialize environment.\n");
		return -1;
	}

	/* track hits, if applicable */
	fsl_hits_init();

	fsl_load_memo();

	tool_ret = tool_entry(argc-2, argv+2);
	fsl_hits_uninit();

	fsl_rt_dump_stats(fsl_env);
	fsl_rt_uninit(fsl_env);

	return tool_ret;
}
