#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "runtime.h"
#include "log.h"
#include "io.h"
#include "debug.h"

typedef uint64_t logaddr_t;
#define addr2log(x)	((x) >> 6)
#define log2addr(x)	((x) << 6)
//	addr >>= 3;	/* 3=bits */
//	addr >>= 3;	/* 3 => 8 byte units */


static void fsl_rlog(struct fsl_rt_io* io, uint64_t addr);

bool fsl_rlog_contains(struct fsl_rt_io* io, diskoff_t addr)
{
	struct fsl_rt_rlog*	log;
	int			i;
	logaddr_t 		logaddr;

	log = &io->io_rlog;
	logaddr = addr2log(addr);
	for (i = 0; i < log->log_accessed_idx; i++) {
		if (log->log_accessed[i] == logaddr)
			return true;
	}

	return false;
}

void fsl_rlog_start(struct fsl_rt_io* io)
{
	fsl_io_callback	old_io_cb;

	FSL_ASSERT (io->io_rlog.log_accessed_idx == IO_IDX_STOPPED);

	old_io_cb = fsl_io_hook(io, fsl_rlog, IO_CB_CACHE_ANY);
	io->io_rlog.log_accessed_idx = 0;
}

void fsl_rlog_stop(struct fsl_rt_io* io)
{
	FSL_ASSERT (io->io_rlog.log_accessed_idx != IO_IDX_STOPPED);
	io->io_rlog.log_accessed_idx = IO_IDX_STOPPED;

	fsl_io_unhook(io, IO_CB_CACHE_ANY);
	if (io->io_rlog.log_next_cb != NULL) {
		fsl_io_hook(io, io->io_rlog.log_next_cb, IO_CB_CACHE_ANY);
		io->io_rlog.log_next_cb = NULL;
	}
}

static void fsl_rlog(struct fsl_rt_io* io, diskoff_t addr)
{
	struct fsl_rt_rlog	*log;

	log = &io->io_rlog;
	FSL_ASSERT (log->log_accessed_idx != IO_IDX_STOPPED);
	FSL_ASSERT (log->log_accessed_idx < IO_MAX_ACCESS);

	/* round down to bytes */
	if (fsl_rlog_contains(io, addr) == false) {
		log->log_accessed[log->log_accessed_idx++] = addr2log(addr);
	}

	if (log->log_next_cb != NULL) log->log_next_cb(io, addr);
}

void fsl_rlog_init(struct fsl_rt_io* io)
{
	io->io_rlog.log_accessed_idx = IO_IDX_STOPPED;
	io->io_rlog.log_next_cb = NULL;
}

void fsl_wlog_init(struct fsl_rt_wlog* wl)
{
	memset(wl, 0, sizeof(struct fsl_rt_wlog));
	wl->wl_idx = -1;
}

void fsl_wlog_start(struct fsl_rt_wlog* wl)
{
	FSL_ASSERT (wl->wl_idx == -1);
	wl->wl_idx = 0;
}

void fsl_wlog_commit(struct fsl_rt_wlog* wl)
{
	int			i;

	FSL_ASSERT (wl->wl_idx >= 0);
	for (i = 0; i < wl->wl_idx; i++) {
		struct fsl_rt_wlog_ent	*we;
		we = &wl->wl_write[i];
		fsl_io_write(we->we_bit_addr, we->we_val, we->we_bits);
	}
	wl->wl_idx = 0;
}

void fsl_wlog_stop(struct fsl_rt_wlog* wl)
{
	FSL_ASSERT (wl->wl_idx == 0 && "STOPPING WITHOUT COMMIT");
	wl->wl_idx = -1;
}

void fsl_wlog_add(struct fsl_rt_wlog* wl, bitoff_t off, uint64_t val, int len)
{
	struct fsl_rt_wlog_ent	*we;

	FSL_ASSERT (wl->wl_idx >= 0 && "ADDING TO STOPPED WLOG");

	/* whoops  */
	if (wl->wl_idx == IO_MAX_ACCESS) {
		FSL_ASSERT (0 == 1 && "PAST MAX ACCESS AIEE");
	}

	/* XXX TODO:  put checks for concurrent write */
	we = &wl->wl_write[wl->wl_idx];
	we->we_bit_addr = off;
	we->we_val = val;
	we->we_bits = len;

	wl->wl_idx++;
}

void fsl_wlog_copy(struct fsl_rt_wlog* dst, const struct fsl_rt_wlog* src)
{
	memcpy(dst, src, sizeof(struct fsl_rt_wlog));
}
