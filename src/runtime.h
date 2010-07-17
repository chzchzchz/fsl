#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>

/* XXX TODO Needs local context for multi-threading.. */


struct fsl_rt_ctx
{
	unsigned int	fctx_num_types;
	uint64_t	*fctx_type_offsets;
	FILE		*fctx_backing;
};

/* exposed to llvm */
uint64_t __getLocal(uint64_t bit_off, uint64_t num_bits);
uint64_t __getLocalArray(
	uint64_t idx, uint64_t bits_in_type, 
	uint64_t base_offset, uint64_t bits_in_array);
uint64_t __getDyn(uint64_t type_num);
void __setDyn(uint64_t type_num, uint64_t offset);
uint64_t __max2(
	uint64_t a0, uint64_t a1);
uint64_t __max3(
	uint64_t a0, uint64_t a1, uint64_t a2);
uint64_t __max4(
	uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3);
uint64_t __max5(
	uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3,
	uint64_t a4);
uint64_t fsl_fail_bits(void);
uint64_t fsl_fail_bytes(void);

/* not exposed to llvm */
struct fsl_rt_ctx* fsl_rt_init(const char* fsl_rt);
void fsl_rt_uninit(struct fsl_rt_ctx* ctx);

#endif
