#ifndef FSL_CACHE_H
#define FSL_CACHE_H

#define FSL_IO_CACHE_ENTS	256	/* 8KB */
#define FSL_IO_CACHE_BYTES	32
#define FSL_IO_CACHE_BITS	(FSL_IO_CACHE_BYTES*8)
#define byte_to_line(x)		((x)/FSL_IO_CACHE_BYTES)
#define bit_to_line(x)		((x)/FSL_IO_CACHE_BITS)
#define byte_to_cache_addr(x)	byte_to_line(x)*FSL_IO_CACHE_BITS
struct fsl_io_cache_ent
{
	uint64_t	ce_addr;	/* line address */
	uint8_t		ce_data[FSL_IO_CACHE_BYTES];
};

struct fsl_io_cache
{
	uint64_t		ioc_misses;
	uint64_t		ioc_hits;
	struct fsl_io_cache_ent	ioc_ents[FSL_IO_CACHE_ENTS];
};

void fsl_io_cache_init(struct fsl_io_cache* ioc);
uint64_t fsl_io_cache_get(struct fsl_rt_io* io, uint64_t bit_off, int num_bits);
void fsl_io_cache_drop_bytes(
	struct fsl_rt_io* io, uint64_t byte_off, uint64_t num_bytes);

#endif
