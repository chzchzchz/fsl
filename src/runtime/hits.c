#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "runtime.h"
#include "io.h"

/* this should probably always be equal to the cache line size ... */
#define HIT_BYTE_GRANULARITY	32

struct io_ev_log
{
	uint64_t	*ev_log;
	const char	*ev_fname;
	const char	*ev_envvar;
	fsl_io_callback	ev_cb;
	fsl_io_callback	ev_cb_prev;
	int		ev_cb_type;
};

#define DECL_CB(x,y,z)		\
static void fsl_hits_io_cb_##x(struct fsl_rt_io* io, uint64_t bit_addr); \
static struct io_ev_log log_##x = {					\
	.ev_envvar = z, .ev_cb = fsl_hits_io_cb_##x, .ev_cb_type = y };	\
static void fsl_hits_io_cb_##x(struct fsl_rt_io* io, uint64_t bit_addr)	\
{									\
	bit_addr /= 8;				/* to bytes */		\
	bit_addr /= HIT_BYTE_GRANULARITY;	/* to sectors */	\
	log_##x.ev_log[bit_addr]++;					\
	log_##x.ev_cb_prev(io, bit_addr);				\
}

DECL_CB(hit, IO_CB_CACHE_HIT, FSL_ENV_VAR_HITFILE)
DECL_CB(miss, IO_CB_CACHE_MISS, FSL_ENV_VAR_MISSFILE)
DECL_CB(write, IO_CB_WRITE, FSL_ENV_VAR_WRITEFILE)

#ifndef USE_KLEE
static void fsl_hits_init_ev(struct io_ev_log* ev, uint64_t byte_len)
{
	ev->ev_log = NULL;
	ev->ev_fname = getenv(ev->ev_envvar);
	if (ev->ev_fname == NULL) return;

	ev->ev_log = malloc(byte_len);

	ev->ev_cb_prev = fsl_io_hook(fsl_get_io(), ev->ev_cb, ev->ev_cb_type);
	if (ev->ev_cb_prev == NULL) ev->ev_cb_prev = fsl_io_cb_identity;
}

static void fsl_hits_uninit_ev(struct io_ev_log* ev)
{
	if (ev->ev_log != NULL) free(ev->ev_log);
	ev->ev_log = NULL;
	ev->ev_fname = NULL;
}
#endif

void fsl_hits_init(void)
{
#ifndef USE_KLEE
	unsigned int	byte_len;

	byte_len = (__FROM_OS_BDEV_BYTES/HIT_BYTE_GRANULARITY+1);
	byte_len *= sizeof(uint64_t);
	fsl_hits_init_ev(&log_hit, byte_len);
	fsl_hits_init_ev(&log_miss, byte_len);
	fsl_hits_init_ev(&log_write, byte_len);
#endif
}

#ifndef USE_KLEE
static void fsl_hits_dump(const struct io_ev_log* ev)
{
	unsigned int	num_ents;
	unsigned int	i;
	FILE		*f;
	const char	*fname;
	uint64_t	*hits;

	fname = ev->ev_fname;
	hits = ev->ev_log;
	if (fname == NULL) return;
	if (hits == NULL) return;

	f = fopen(fname, "w");
	if (f == NULL) {
		fprintf(stderr, "Could not open hit file '%s'\n", fname);
		return;
	}

	num_ents = __FROM_OS_BDEV_BYTES/HIT_BYTE_GRANULARITY;
	for (i = 0; i < num_ents; i++) {
		uint64_t	addr;

		if (hits[i] == 0) continue;

		addr = (uint64_t)i * HIT_BYTE_GRANULARITY;
		fprintf(f,
			"{ 'start_byte_offset' : %"PRIu64
				", 'last_byte_offset' : %"PRIu64
				", 'hits' : %"PRIu64
				"}\n",
			addr,
			addr + HIT_BYTE_GRANULARITY - 1,
			hits[i]);
	}

	fclose(f);
}
#endif

void fsl_hits_uninit(void)
{
#ifndef USE_KLEE
	fsl_hits_dump(&log_hit);
	fsl_hits_dump(&log_miss);
	fsl_hits_dump(&log_write);

	fsl_hits_uninit_ev(&log_hit);
	fsl_hits_uninit_ev(&log_miss);
	fsl_hits_uninit_ev(&log_write);
#endif
}
