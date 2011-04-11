#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "runtime.h"
#include "io.h"

#define LOG_BUF_ENTS		4096

struct cb_log
{
	/* static initialized data */
	const char	*cl_envvar;
	fsl_io_callback	cl_cb;
	int		cl_cb_type;
	/* dyn data */
	uint64_t	cl_buf[LOG_BUF_ENTS];
	unsigned int	cl_buf_idx;
	const char	*cl_fname;
	FILE		*cl_file;
	fsl_io_callback	cl_cb_prev;
};

static void log_update(struct cb_log* l, uint64_t bit_addr);
static void log_flush(struct cb_log* l);

#define DECL_CB(x,y,z)		\
static void fsl_temp_io_cb_##x(struct fsl_rt_io* io, uint64_t bit_addr); \
static struct cb_log log_##x = {					\
	.cl_envvar = z, .cl_cb = fsl_temp_io_cb_##x, .cl_cb_type = y };	\
static void fsl_temp_io_cb_##x(struct fsl_rt_io* io, uint64_t bit_addr)	\
{									\
	log_update(&log_##x, bit_addr);					\
	log_##x.cl_cb_prev(io, bit_addr);				\
}

DECL_CB(hit, IO_CB_CACHE_HIT, FSL_ENV_VAR_TEMPORAL_HITFILE)
DECL_CB(miss, IO_CB_CACHE_MISS, FSL_ENV_VAR_TEMPORAL_MISSFILE)
DECL_CB(write, IO_CB_WRITE, FSL_ENV_VAR_TEMPORAL_WRITEFILE)

static void log_update(struct cb_log* cl, uint64_t bit_addr)
{
	cl->cl_buf[cl->cl_buf_idx++] = bit_addr;
	if (cl->cl_buf_idx == LOG_BUF_ENTS) log_flush(cl);
}

static void log_flush(struct cb_log* cl)
{
	unsigned int	i;
	for (i = 0; i < cl->cl_buf_idx; i++)
		fprintf(cl->cl_file, "%"PRIu64"\n", cl->cl_buf[i]);
	cl->cl_buf_idx = 0;
}

#ifndef USE_KLEE
static void log_uninit(struct cb_log* cl)
{
	if (cl->cl_file == NULL) return;
	log_flush(cl);
	fclose(cl->cl_file);
}

static void log_init(struct cb_log* cl)
{
	cl->cl_fname = getenv(cl->cl_envvar);
	if (cl->cl_fname == NULL) return;
	cl->cl_file = fopen(cl->cl_fname, "w");
	FSL_ASSERT(cl->cl_file != NULL);

	cl->cl_cb_prev = fsl_io_hook(fsl_get_io(), cl->cl_cb, cl->cl_cb_type);
	if (cl->cl_cb_prev == NULL) cl->cl_cb_prev = fsl_io_cb_identity;
}
#endif

void fsl_temporal_init(void)
{
#ifndef USE_KLEE
	log_init(&log_hit);
	log_init(&log_miss);
	log_init(&log_write);
#endif
}

void fsl_temporal_uninit(void)
{
#ifndef USE_KLEE
	log_uninit(&log_hit);
	log_uninit(&log_miss);
	log_uninit(&log_write);
#endif
}
