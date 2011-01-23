#ifndef FSL_IO_H
#define FSL_IO_H

#include "runtime.h"
#include "log.h"

#define fsl_get_io()		(fsl_env->fctx_io)

#define IO_CB_CACHE_HIT		0	/* handled by our cache */
#define IO_CB_CACHE_MISS	1	/* call out to OS */
#define IO_CB_CACHE_ANY		2
#define IO_CB_WRITE		3
#define IO_CB_NUM		4

struct fsl_rt_io_priv;

struct fsl_rt_io
{
	union {
		fsl_io_callback	io_cb[IO_CB_NUM];
		struct {
			fsl_io_callback		io_cb_hit;
			fsl_io_callback		io_cb_miss;
			fsl_io_callback		io_cb_any;
			fsl_io_callback		io_cb_write;
		};
	};
	struct fsl_rt_rlog	io_rlog;
	struct fsl_rt_wlog	io_wlog;
	uint32_t		io_blksize;
	ssize_t			io_blk_c;
	struct fsl_rt_io_priv	*io_priv;
};

struct fsl_rt_closure;
struct fsl_rt_io;
struct fsl_rt_wlog;

/* exposed to fsl llvm */
uint64_t __getLocal(
	const struct fsl_rt_closure*, uint64_t bit_off, uint64_t num_bits);
uint64_t __getLocalPhys(uint64_t bit_off, uint64_t num_bits);
uint64_t __getLocalArray(
	const struct fsl_rt_closure*,
	uint64_t idx, uint64_t bits_in_type,
	uint64_t base_offset, uint64_t bits_in_array);
void __writeVal(uint64_t loc, uint64_t sz, uint64_t val);

void fsl_io_steal_wlog(struct fsl_rt_io* io, struct fsl_rt_wlog* dst);
void fsl_io_abort_wlog(struct fsl_rt_io* io);

/* io funcs */
struct fsl_rt_io* fsl_io_alloc(const char* backing_fname);
void fsl_io_free(struct fsl_rt_io* io);
ssize_t fsl_io_size(struct fsl_rt_io* io);
void fsl_io_write(uint64_t bit_off, uint64_t val, uint64_t num_bits);
fsl_io_callback fsl_io_hook(struct fsl_rt_io* io, fsl_io_callback, int cb_type);
void fsl_io_unhook(struct fsl_rt_io* io, int cb_type);
void fsl_io_do_wpkt(const struct fsl_rtt_wpkt* wpkt, const uint64_t* params);

void fsl_io_read_bytes(void* buf, unsigned int byte_c, uint64_t off);
void fsl_io_write_bytes(void* buf, unsigned int byte_c, uint64_t off);

void fsl_io_dump_pending(void);

#endif
