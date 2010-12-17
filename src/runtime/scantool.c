//#define DEBUG_TOOL
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include "runtime.h"
#include "debug.h"
#include "type_info.h"
#include "scan.h"

#define INDENT(x)	print_indent(ti_depth(x))

static void print_indent(unsigned int depth)
{
	while (depth) {
		printf(" ");
		depth--;
	}
}

static void dump_field(const struct fsl_rt_table_field* field, diskoff_t bitoff)
{
	printf("%s::", field->tf_fieldname);
	printf("%s",
		(field->tf_typenum != TYPENUM_INVALID) ?
			tt_by_num(field->tf_typenum)->tt_name :
			"ANONYMOUS");

	printf("@offset=0x%" PRIx64 " (%" PRIu64 ")\n", bitoff, bitoff);
}

static int handle_strong(
	struct type_info* ti,
	const struct fsl_rt_table_field* field, void* aux)
{
	size_t		off;
	INDENT(ti);
	off = field->tf_fieldbitoff(&ti_clo(ti));
	dump_field(field, off);
	return SCAN_RET_CONTINUE;
}

static int handle_ti(struct type_info* ti, void* aux)
{
	typesize_t	size;
	voff_t		voff;
	poff_t		poff;

	DEBUG_TOOL_ENTER();
	if (ti_typenum(ti) == TYPENUM_INVALID) goto done;

	voff = ti_offset(ti);
	poff = ti_phys_offset(ti);
	size = ti_size(ti);

	DEBUG_TOOL_WRITE("poff=%"PRIu64, poff);
	assert (offset_in_range(poff) && "NOT ON DISK?");

#ifdef DEBUG_TOOL
	typeinfo_print_path(ti);
	printf("\n");
#endif

	printf("{ 'Mode' : 'Scan', 'voff' : %"PRIu64
		", 'poff' : %"PRIu64
		", 'size': %"PRIu64
		", 'name' : '%s' "
		", 'depth' : %d }\n",
		voff, poff, size,
		tt_by_ti(ti)->tt_name,
		ti_depth(ti));

done:
	DEBUG_TOOL_LEAVE();
	return SCAN_RET_CONTINUE;
}

struct scan_ops ops =
{
	.so_ti = handle_ti,
	.so_strong = handle_strong
};

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;
	struct type_desc	init_td = td_origin();

	printf("Welcome to fsl scantool. Scan mode: \"%s\"\n", fsl_rt_fsname);

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_by_field(&init_td, NULL, NULL);
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		return -1;
	}

	DEBUG_TOOL_WRITE("Origin Type Now Allocated");

	scan_type(origin_ti, &ops, NULL);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}
