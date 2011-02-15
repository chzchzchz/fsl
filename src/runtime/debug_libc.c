#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "debug.h"

unsigned int fsl_debug_depth = 0;

static void fsl_debug_indent(void);

void fsl_debug_enter(const char *func_name)
{
	fsl_debug_indent();
	printf("%s: entering\n", func_name);
	fsl_debug_indent();
	printf("{\n");
	fsl_debug_depth++;
}

void fsl_debug_leave(const char *func_name)
{
	fsl_debug_indent();
	printf("%s: leaving\n", func_name);
	fsl_debug_depth--;
	fsl_debug_indent();
	printf("}\n");
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
	for (i = 0; i < fsl_debug_depth; i++) printf(" ");
}

void fsl_debug_vwrite(const char* fmt, va_list vl)
{
	fsl_debug_indent();
	vprintf(fmt, vl);
	printf("\n");
}

void fsl_assert_fail(const char* fname, int line, const char* str)
{
	printf("BARF: %s@%d. %s\n", fname, line, str);
	exit(-1);
}
