#ifndef FSL_IO_H
#define FSL_IO_H

#define fsl_get_io()		(fsl_env->fctx_io)

struct fsl_rt_closure;
struct fsl_rt_io;
struct fsl_rtt_wpkt;

typedef void (*fsl_io_callback)(struct fsl_rt_io*, uint64_t /* bit off */);

#define IO_MAX_ACCESS		64
struct fsl_rt_rlog
{
	byteoff_t		log_accessed[IO_MAX_ACCESS];
	int			log_accessed_idx;
	fsl_io_callback		log_next_cb;
};

struct fsl_rt_wlog_ent
{
	uint64_t	we_bit_addr;
	uint64_t	we_val;
	uint8_t		we_bits;
};

struct fsl_rt_wlog
{
	struct fsl_rt_wlog_ent	wl_write[IO_MAX_ACCESS];
	int			wl_idx;
};

#define FSL_IO_CACHE_ENTS	64
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

#define IO_CB_CACHE_HIT		0	/* handled by our cache */
#define IO_CB_CACHE_MISS	1	/* call out to OS */
#define IO_CB_CACHE_ANY		2
#define IO_CB_WRITE		3
#define IO_CB_NUM		4

struct fsl_rt_io
{
	int			io_fd;
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
	struct fsl_io_cache	io_cache;
};

/* exposed to fsl llvm */
uint64_t __getLocal(
	const struct fsl_rt_closure*, uint64_t bit_off, uint64_t num_bits);
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
