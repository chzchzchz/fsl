#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "runtime.h"
#include "io.h"

#define HIT_BYTE_GRANULARITY	512

static uint64_t*	hit_log;
static const char*	fname;

static void fsl_hits_io_cb(struct fsl_rt_io* io, uint64_t bit_addr)
{
	bit_addr /= 8;				/* to bytes */
	bit_addr /= HIT_BYTE_GRANULARITY;	/* to sectors */
	hit_log[bit_addr]++;
}

void fsl_hits_init(void)
{
	fsl_io_callback	old_cb;
	unsigned int	byte_len;

	fname = getenv(FSL_ENV_VAR_HITFILE);
	if (fname == NULL)
		return;

	byte_len = sizeof(uint64_t) *
		(__FROM_OS_BDEV_BYTES/HIT_BYTE_GRANULARITY+1);
	hit_log = malloc(byte_len);
	memset(hit_log, 0, byte_len);

	old_cb = fsl_io_hook(fsl_get_io(), fsl_hits_io_cb);
	assert (old_cb != NULL && "No nesting with hits!");
}

void fsl_hits_uninit(void)
{
	FILE		*f;
	unsigned int	i;
	unsigned int	num_ents;

	if (fname == NULL)	return;
	if (hit_log == NULL)	return;

	f = fopen(fname, "w");
	if (f == NULL) {
		fprintf(stderr, "Could not open hit file\n");
		free(hit_log);
		return;
	}

	num_ents = __FROM_OS_BDEV_BYTES/HIT_BYTE_GRANULARITY;
	for (i = 0; i < num_ents; i++) {
		uint64_t	addr;

		if (hit_log[i] == 0)
			continue;

		addr = (uint64_t)i * HIT_BYTE_GRANULARITY;
		fprintf(f,
			"{ 'start_byte_offset' : %"PRIu64
				", 'last_byte_offset' : %"PRIu64
				", 'hits' : %"PRIu64
				"}\n",
			addr,
			addr + HIT_BYTE_GRANULARITY - 1,
			hit_log[i]);
	}

	fclose(f);

	free(hit_log);
	fname = NULL;
	hit_log = NULL;
}
