#ifndef FSL_CACHE_H
#define FSL_CACHE_H

void fsl_io_cache_init(struct fsl_io_cache* ioc);
const uint8_t* fsl_io_cache_find(struct fsl_io_cache* io, uint64_t cache_line);
uint64_t fsl_io_cache_get(struct fsl_rt_io* io, uint64_t bit_off, int num_bits);
void fsl_io_cache_drop_bytes(
	struct fsl_rt_io* io,
	uint64_t byte_off, uint64_t num_bytes);

#endif
