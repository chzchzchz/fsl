#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "runtime.h"
#include "io.h"

#define HIT_BYTE_GRANULARITY	512

static uint64_t*	hit_log;
static uint64_t*	miss_log;
static const char*	fname_hit;
static const char*	fname_miss;

static void fsl_hits_io_cb_hit(struct fsl_rt_io* io, uint64_t bit_addr)
{
	bit_addr /= 8;				/* to bytes */
	bit_addr /= HIT_BYTE_GRANULARITY;	/* to sectors */
	hit_log[bit_addr]++;
}

static void fsl_hits_io_cb_miss(struct fsl_rt_io* io, uint64_t bit_addr)
{
	bit_addr /= 8;				/* to bytes */
	bit_addr /= HIT_BYTE_GRANULARITY;	/* to sectors */
	miss_log[bit_addr]++;
}


static void fsl_hits_init_hit(uint64_t byte_len)
{
	fsl_io_callback	old_cb;

	fname_hit = getenv(FSL_ENV_VAR_HITFILE);
	if (fname_hit == NULL)
		return;

	hit_log = malloc(byte_len);

	old_cb = fsl_io_hook(fsl_get_io(), fsl_hits_io_cb_hit, IO_CB_CACHE_HIT);
	assert (old_cb == NULL && "No nesting with hits!");
}

static void fsl_hits_init_miss(uint64_t byte_len)
{
	fsl_io_callback	old_cb;

	fname_miss = getenv(FSL_ENV_VAR_MISSFILE);
	if (fname_miss == NULL)
		return;

	miss_log = malloc(byte_len);

	old_cb = fsl_io_hook(
		fsl_get_io(), fsl_hits_io_cb_miss, IO_CB_CACHE_MISS);
	assert (old_cb == NULL && "No nesting with hits!");
}

void fsl_hits_init(void)
{
	unsigned int	byte_len;

	byte_len = (__FROM_OS_BDEV_BYTES/HIT_BYTE_GRANULARITY+1);
	byte_len *= sizeof(uint64_t);

	fsl_hits_init_hit(byte_len);
	fsl_hits_init_miss(byte_len);
}

static void fsl_hits_dump(const char* fname, uint64_t* hits)
{
	unsigned int	num_ents;
	unsigned int	i;
	FILE		*f;

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

		if (hit_log[i] == 0) continue;

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

void fsl_hits_uninit(void)
{
	fsl_hits_dump(fname_hit, hit_log);
	fsl_hits_dump(fname_miss, miss_log);

	if (hit_log != NULL) free(hit_log);
	if (miss_log != NULL) free(miss_log);

	fname_hit = NULL;
	fname_miss = NULL;

	hit_log = NULL;
	miss_log = NULL;
}
