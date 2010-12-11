#ifndef FSL_LOG_H
#define FSL_LOG_H

#define IO_IDX_STOPPED		(-2)

#include "io.h"

bool fsl_rlog_contains(struct fsl_rt_io* io, diskoff_t addr);
void fsl_rlog_init(struct fsl_rt_io* io);
void fsl_rlog_start(struct fsl_rt_io* io);
void fsl_rlog_stop(struct fsl_rt_io* io);
void fsl_rlog_dump(struct fsl_rt_io* io, FILE* f);

void fsl_wlog_init(struct fsl_rt_wlog* io);
void fsl_wlog_start(struct fsl_rt_wlog* io);
void fsl_wlog_add(struct fsl_rt_wlog* io, bitoff_t off, uint64_t val, int len);
void fsl_wlog_commit(struct fsl_rt_wlog* io);
void fsl_wlog_stop(struct fsl_rt_wlog* io);
void fsl_wlog_copy(struct fsl_rt_wlog* dst, const struct fsl_rt_wlog* src);

#define fsl_rlog_ents(io)	((io)->io_accessed_idx)
#define FSL_LOG_START()	fsl_rlog_start(fsl_get_io())
#define FSL_LOG_STOP()	fsl_rlog_stop(fsl_get_io())
#define FSL_LOG_DUMP()	fsl_rlog_dump(fsl_get_io(),stdout)

#define FSL_WRITE_START()	fsl_wlog_start(&fsl_get_io()->io_wlog)
#define FSL_WRITE_COMPLETE()					\
	do {							\
		fsl_wlog_commit(&fsl_get_io()->io_wlog);	\
		fsl_wlog_stop(&fsl_get_io()->io_wlog);		\
	} while (0)

#endif
