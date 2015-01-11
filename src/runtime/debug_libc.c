#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "debug.h"

unsigned int fsl_debug_depth = 0;
static FILE* debug_out_file = NULL;

#define get_out_f()	((debug_out_file == NULL) ? stdout : debug_out_file)

static void fsl_debug_indent(void);

void fsl_debug_enter(const char *func_name)
{
	fsl_debug_indent();
	fprintf(get_out_f(), "%s: entering\n", func_name);
	fsl_debug_indent();
	fprintf(get_out_f(), "{\n");
	fsl_debug_depth++;
}

void fsl_debug_leave(const char *func_name)
{
	fsl_debug_indent();
	fprintf(get_out_f(), "%s: leaving\n", func_name);
	fsl_debug_depth--;
	fsl_debug_indent();
	fprintf(get_out_f(), "}\n");
}

void fsl_debug_write(const char* fmt, ...)
{
	va_list	vl;
	va_start(vl, fmt);
	fsl_debug_vwrite(fmt, vl);
	va_end(vl);
}

static void fsl_debug_indent(void)
{
	unsigned int i;
	for (i = 0; i < fsl_debug_depth; i++) fprintf(get_out_f(), " ");
}

void fsl_debug_vwrite(const char* fmt, va_list vl)
{
	fsl_debug_indent();
	vfprintf(get_out_f(), fmt, vl);
	fprintf(get_out_f(), "\n");
}

void fsl_assert_fail(const char* fname, int line, const char* str)
{
	fprintf(get_out_f(), "BARF: %s@%d. %s\n", fname, line, str);
	abort();
	exit(-1);
}

void fsl_debug_set_file(FILE* f) { debug_out_file = f; }
