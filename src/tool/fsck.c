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
#include "writepkt.h"

static void do_repair(struct type_info* ti, const struct fsl_rtt_repair* rep)
{
	/* did we data to repair? */
	if (rep->rep_cond(&ti_clo(ti)) == 0) return;

	/* doit */
	FSL_INFO("Repairing %s::%s\n", tt_by_ti(ti)->tt_name, rep->rep_name);
	wpkt_do(ti, &rep->rep_wpkt);
	/* and reload memo table */
	/* XXX: this should be smarter-- handle in runtime! */
	fsl_load_memo();
}

static int handle_ti(struct type_info* ti, void* aux)
{
	const struct fsl_rtt_type*	tt;
	unsigned int			i;

	DEBUG_TOOL_ENTER();
	if (ti_typenum(ti) == TYPENUM_INVALID) goto done;

	tt = tt_by_ti(ti);
	if (tt->tt_repair_c == 0) goto done;

	for (i = 0; i < tt->tt_repair_c; i++)
		do_repair(ti, &tt->tt_repair[i]);

done:
	DEBUG_TOOL_LEAVE();
	return SCAN_RET_CONTINUE;
}

struct scan_ops ops = { .so_ti = handle_ti };

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;

	FSL_INFO("Welcome to fsl fsck. FSCK mode: \"%s\"\n", fsl_rt_fsname);

	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		FSL_INFO("Could not open origin type\n");
		FSL_INFO("Bad assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	scan_type_deep(origin_ti, &ops, NULL);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}