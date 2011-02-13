#ifndef FSLINFO_H
#define FSLINFO_H
#include <stdarg.h>
#define FSL_INFO	fsl_info_write
void fsl_info_write(const char* fmt, ...);
void fsl_info_vwrite(const char* fmt, va_list vl);
#endif
