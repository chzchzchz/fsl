#ifndef FSL_RT_DEBUG_H
#define FSL_RT_DEBUG_H

#include <stdarg.h>

extern unsigned int fsl_debug_depth;

#define DEBUG_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_WRITE	fsl_debug_write

#ifdef DEBUG_TOOL
#define DEBUG_TOOL_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_TOOL_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_TOOL_WRITE	fsl_debug_write
#else
#define DEBUG_TOOL_ENTER()	do {} while (0)
#define DEBUG_TOOL_LEAVE()	do {} while (0)
#define DEBUG_TOOL_WRITE
#endif

#ifdef DEBUG_TYPEINFO
#define DEBUG_TYPEINFO_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_TYPEINFO_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_TYPEINFO_WRITE	fsl_debug_write
#else
#define DEBUG_TYPEINFO_ENTER()	do {} while (0)
#define DEBUG_TYPEINFO_LEAVE()	do {} while (0)
#define DEBUG_TYPEINFO_WRITE
#endif

#ifdef DEBUG_VIRT
#define DEBUG_VIRT_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_VIRT_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_VIRT_WRITE	fsl_debug_write
#else
#define DEBUG_VIRT_ENTER()	do {} while (0)
#define DEBUG_VIRT_LEAVE()	do {} while (0)
#define DEBUG_VIRT_WRITE
#endif



void fsl_debug_enter(const char *func_name);
void fsl_debug_leave(const char *func_name);
void fsl_debug_write(const char* fmt, ...);
void fsl_debug_vwrite(const char* fmt, va_list vl);

#endif
