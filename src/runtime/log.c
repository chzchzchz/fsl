#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include "runtime.h"
#include "log.h"

typedef uint64_t logaddr_t;
#define addr2log(x)	((x) >> 6)
#define log2addr(x)	((x) << 6)
//	addr >>= 3;	/* 3=bits */
//	addr >>= 3;	/* 3 => 8 byte units */


static void fsl_io_log(struct fsl_rt_io* io, uint64_t addr);



bool fsl_io_log_contains(struct fsl_rt_io* io, diskoff_t addr)
{
	struct fsl_rt_log*	log;
	int			i;
	logaddr_t 		logaddr;

	log = &io->io_log;
	logaddr = addr2log(addr);
	for (i = 0; i < log->log_accessed_idx; i++) {
		if (log->log_accessed[i] == logaddr)
			return true;
	}

	return false;
}

void fsl_io_log_start(struct fsl_rt_io* io)
{
	fsl_io_callback	old_io_cb;

	assert (io->io_log.log_accessed_idx == IO_IDX_STOPPED);

	old_io_cb = fsl_io_hook(io, fsl_io_log);
	io->io_log.log_accessed_idx = 0;
}

void fsl_io_log_stop(struct fsl_rt_io* io)
{
	assert (io->io_log.log_accessed_idx != IO_IDX_STOPPED);
	io->io_log.log_accessed_idx = IO_IDX_STOPPED;

	fsl_io_unhook(io);
	if (io->io_log.log_next_cb != NULL) {
		fsl_io_hook(io, io->io_log.log_next_cb);
		io->io_log.log_next_cb = NULL;
	}
}

static void fsl_io_log(struct fsl_rt_io* io, diskoff_t addr)
{
	struct fsl_rt_log	*log;

	log = &io->io_log;
	assert (log->log_accessed_idx != IO_IDX_STOPPED);
	assert (log->log_accessed_idx < IO_MAX_ACCESS);

	/* round down to bytes */
	if (fsl_io_log_contains(io, addr) == false) {
		log->log_accessed[log->log_accessed_idx++] = addr2log(addr);
	}

	if (log->log_next_cb != NULL) log->log_next_cb(io, addr);
}

void fsl_io_log_dump(struct fsl_rt_io* io, FILE* f)
{
	unsigned int	i;

	for (i = 0; i < io->io_log.log_accessed_idx; i++) {
		logaddr_t	logaddr;

		logaddr = io->io_log.log_accessed[i];
		fprintf(f, "%d. [%"PRIu64"--%"PRIu64"]\n",
			i,
			log2addr(logaddr),
			log2addr(logaddr+1)-1);
	}
}

void fsl_io_log_init(struct fsl_rt_io* io)
{
	io->io_log.log_accessed_idx = IO_IDX_STOPPED;
	io->io_log.log_next_cb = NULL;
}
