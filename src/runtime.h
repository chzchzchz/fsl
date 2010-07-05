#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>

/* XXX TODO Needs local context for multi-threading.. */

struct fsl_rt_ctx
{
	/* maybe a type-stack? I don't know. */
};

static uint64_t __getLocal(uint64_t off, uint64_t sz);
static uint64_t __getLocalArray(
	uint64_t idx, uint64_t bits_in_type, 
	uint64_t base_offset, uint64_t bits_in_array);
static uint64_t __arg(uint64_t arg_num);
static uint64_t __getDyn(uint64_t type_num, uint64_t offset, uint64_t sz);

#endif
