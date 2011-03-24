//#define DEBUG_TOOL
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include "info.h"
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

	assert (offset_in_range(poff) && "NOT ON DISK?");

	printf("fsck XXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
	exit(-1);

done:
	DEBUG_TOOL_LEAVE();
	return SCAN_RET_CONTINUE;
}

struct scan_ops ops = { .so_ti = handle_ti };

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;

	printf("Welcome to fsl fsck. FSCK mode: \"%s\"\n", fsl_rt_fsname);

	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		fsl_info_write("Could not open origin type\n");
		fsl_info_write("Bad assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	scan_type(origin_ti, &ops, NULL);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}