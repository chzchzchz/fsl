#ifndef FSL_LOG_H
#define FSL_LOG_H

#define IO_IDX_STOPPED		(-2)

#include "io.h"

bool fsl_io_log_contains(struct fsl_rt_io* io, diskoff_t addr);
void fsl_io_log_init(struct fsl_rt_io* io);
void fsl_io_log_start(struct fsl_rt_io* io);
void fsl_io_log_stop(struct fsl_rt_io* io);
void fsl_io_log_dump(struct fsl_rt_io* io, FILE* f);
#define fsl_io_log_ents(io)	((io)->io_accessed_idx)
#define FSL_LOG_START()	fsl_io_log_start(fsl_get_io())
#define FSL_LOG_STOP()	fsl_io_log_stop(fsl_get_io())
#define FSL_LOG_DUMP()	fsl_io_log_dump(fsl_get_io(),stdout)



#endif
