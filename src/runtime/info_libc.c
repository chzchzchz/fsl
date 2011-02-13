#include "info.h"

void fsl_info_write(const char* fmt, ...)
{
	va_list	vl;
	va_start(vl, fmt);
	fsl_info_vwrite(fmt, vl);
	va_end(vl);
}

void fsl_info_vwrite(const char* fmt, va_list vl) { vprintf(fmt, vl); }