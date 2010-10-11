#ifndef FSL_IO_H
#define FSL_IO_H

#define fsl_get_io()		(fsl_env->fctx_io)

struct fsl_rt_closure;
struct fsl_rt_io;

typedef void (*fsl_io_callback)(struct fsl_rt_io*, uint64_t /* bit off */);

#define IO_MAX_ACCESS		64
struct fsl_rt_log
{
	byteoff_t		log_accessed[IO_MAX_ACCESS];
	int			log_accessed_idx;
	fsl_io_callback		log_next_cb;
};

struct fsl_rt_io
{
	FILE			*io_backing;
	fsl_io_callback		io_cb;
	struct fsl_rt_log	io_log;
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
fsl_io_callback fsl_io_hook(struct fsl_rt_io* io, fsl_io_callback);
void fsl_io_unhook(struct fsl_rt_io* io);

#endif
