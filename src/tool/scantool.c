//#define DEBUG_TOOL
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include "runtime.h"
#include "debug.h"
#include "type_info.h"
#include "scan.h"

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

#ifndef USE_KLEE
	printf("{ 'Mode' : 'Scan', 'voff' : %"PRIu64
		", 'poff' : %"PRIu64
		", 'size': %"PRIu64
		", 'name' : '%s' "
		", 'depth' : %d }\n",
		voff, poff, size,
		tt_by_ti(ti)->tt_name,
		ti_depth(ti));
#endif
done:
	DEBUG_TOOL_LEAVE();
	return SCAN_RET_CONTINUE;
}

struct scan_ops ops = { .so_ti = handle_ti };

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;

	printf("Welcome to fsl scantool. Scan mode: \"%s\"\n", fsl_rt_fsname);

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		printf("Failed assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	DEBUG_TOOL_WRITE("Origin Type Now Allocated");

	scan_type(origin_ti, &ops, NULL);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}
