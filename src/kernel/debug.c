#include <linux/kernel.h>
#include "../runtime/debug.h"

unsigned int fsl_debug_depth = 0;

static void fsl_debug_indent(void);

void fsl_debug_enter(const char *func_name)
{
	fsl_debug_indent();
	printk("%s: entering\n", func_name);
	fsl_debug_indent();
	printk("{\n");
	fsl_debug_depth++;
}

void fsl_debug_leave(const char *func_name)
{
	fsl_debug_indent();
	printk("%s: leaving\n", func_name);
	fsl_debug_depth--;
	fsl_debug_indent();
	printk("}\n");
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
	for (i = 0; i < fsl_debug_depth; i++)
		printk(" ");
}

void fsl_debug_vwrite(const char* fmt, va_list vl)
{
	fsl_debug_indent();
	vprintk(fmt, vl);
	printk("\n");
}

void fsl_assert_fail(const char* fname, int line, const char* str)
{
	printk("BARF: %s@%d. %s\n", fname, line, str);
	BUG_ON(1);
}
