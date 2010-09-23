#ifndef FSL_IO_H
#define FSL_IO_H

#define IO_MAX_ACCESS		64
#define IO_IDX_STOPPED		(-2)

#define fsl_get_io()		(fsl_env->fctx_io)

struct fsl_rt_closure;

struct fsl_rt_io
{
	FILE			*io_backing;
	byteoff_t		io_accessed[IO_MAX_ACCESS];
	int			io_accessed_idx;
};

/* exposed to fsl llvm */
uint64_t __getLocal(
	const struct fsl_rt_closure*, uint64_t bit_off, uint64_t num_bits);
uint64_t __getLocalArray(
	const struct fsl_rt_closure*,
	uint64_t idx, uint64_t bits_in_type,
	uint64_t base_offset, uint64_t bits_in_array);

/* io funcs */
struct fsl_rt_io* fsl_io_alloc(const char* backing_fname);
void fsl_io_free(struct fsl_rt_io* io);
ssize_t fsl_io_size(struct fsl_rt_io* io);

bool fsl_io_log_contains(struct fsl_rt_io* io, diskoff_t addr);
void fsl_io_log_start(struct fsl_rt_io* io);
void fsl_io_log_stop(struct fsl_rt_io* io);
void fsl_io_log_dump(struct fsl_rt_io* io, FILE* f);
#define fsl_io_log_ents(io)	((io)->io_accessed_idx)
#define FSL_LOG_START()	fsl_io_log_start(fsl_get_io())
#define FSL_LOG_STOP()	fsl_io_log_stop(fsl_get_io())
#define FSL_LOG_DUMP()	fsl_io_log_dump(fsl_get_io(),stdout)

#endif
