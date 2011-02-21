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
#define DEBUG_TOOL_WRITE(x, ...)
#endif

#ifdef DEBUG_CHOICE
#define DEBUG_CHOICE_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_CHOICE_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_CHOICE_WRITE	fsl_debug_write
#else
#define DEBUG_CHOICE_ENTER()	do {} while (0)
#define DEBUG_CHOICE_LEAVE()	do {} while (0)
#define DEBUG_CHOICE_WRITE(x, ...)
#endif

#ifdef DEBUG_TYPEINFO
#define DEBUG_TYPEINFO_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_TYPEINFO_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_TYPEINFO_WRITE	fsl_debug_write
#else
#define DEBUG_TYPEINFO_ENTER()	do {} while (0)
#define DEBUG_TYPEINFO_LEAVE()	do {} while (0)
#define DEBUG_TYPEINFO_WRITE(x, ...)
#endif

#ifdef DEBUG_VIRT
#define DEBUG_VIRT_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_VIRT_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_VIRT_WRITE	fsl_debug_write
#else
#define DEBUG_VIRT_ENTER()	do {} while (0)
#define DEBUG_VIRT_LEAVE()	do {} while (0)
#define DEBUG_VIRT_WRITE(x, ...)
#endif

#ifdef DEBUG_IO
#define DEBUG_IO_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_IO_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_IO_WRITE		fsl_debug_write
#else
#define DEBUG_IO_ENTER()	do {} while (0)
#define DEBUG_IO_LEAVE()	do {} while (0)
#define DEBUG_IO_WRITE(x,...)
#endif

#ifdef DEBUG_RT
#define DEBUG_RT_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_RT_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_RT_WRITE		fsl_debug_write
#else
#define DEBUG_RT_ENTER()	do {} while (0)
#define DEBUG_RT_LEAVE()	do {} while (0)
#define DEBUG_RT_WRITE(x, ...)
#endif

#ifdef DEBUG_SCAN
#define DEBUG_SCAN_ENTER()	fsl_debug_enter(__FUNCTION__)
#define DEBUG_SCAN_LEAVE()	fsl_debug_leave(__FUNCTION__)
#define DEBUG_SCAN_WRITE	fsl_debug_write
#else
#define DEBUG_SCAN_ENTER()	do {} while (0)
#define DEBUG_SCAN_LEAVE()	do {} while (0)
#define DEBUG_SCAN_WRITE(x, ...)
#endif

typedef unsigned int statenum_t;

void fsl_debug_enter(const char *func_name);
void fsl_debug_leave(const char *func_name);
void fsl_debug_write(const char* fmt, ...);
void fsl_debug_vwrite(const char* fmt, va_list vl);
void fsl_debug_return_to_state(statenum_t);
void fsl_assert_fail(const char* fname, int line, const char* str);
#ifndef __KERNEL__
#include <stdio.h>
void fsl_debug_set_file(FILE* f);
#endif
#define fsl_debug_get_state() fsl_debug_depth

#define FSL_ASSERT(x)	do {				\
	if (!__builtin_expect((x), 1)) fsl_assert_fail(__FILE__, __LINE__, #x);	\
	} while (0);

#endif
