//#define DEBUG_TOOL
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include "runtime.h"
#include "debug.h"
#include "type_info.h"
#include "scan.h"

struct fragscan_info
{
	unsigned int	virt_c;
};

static int handle_virt(
	struct type_info* ti,
	const struct fsl_rtt_virt* vt, int idx, struct fragscan_info* info)
{
	const struct fsl_rt_mapping	*rtm;
	struct type_info		*v_ti;
	size_t				elem_len;
	uint64_t			last_poff;
	unsigned int			cur_elem, total_elems;
	int				err;

	if (idx != 0) return SCAN_RET_CONTINUE;

	v_ti = typeinfo_follow_virt(ti, vt, idx, &err);
	if (v_ti == NULL) return SCAN_RET_CONTINUE;
	rtm = ti_xlate(v_ti);
	assert (rtm != NULL && "Vtype => xlate!");

	/* read offset for each element */
	elem_len = fsl_virt_elem_bits(rtm);
	total_elems = fsl_virt_total_bits(rtm) / elem_len;
	last_poff = fsl_virt_xlate(&ti_clo(v_ti), 0);
	for (cur_elem = 1; cur_elem < total_elems; cur_elem++) {
		uint64_t	cur_poff;

		cur_poff = fsl_virt_xlate(&ti_clo(v_ti), cur_elem*elem_len);
		if (cur_poff != (elem_len + last_poff)) {
			printf(	"{ 'vname' : '%s', 'elemidx' : %d, 'id' : %d,"
				" 'lastpoff' : %"PRIu64", "
				" 'curpoff' : %"PRIu64"}\n",
				v_ti->ti_virt->vt_name, cur_elem, info->virt_c,
				last_poff,
				cur_poff);
		}
		last_poff = cur_poff;
	}

	info->virt_c++;

	return SCAN_RET_CONTINUE;
}

struct scan_ops ops =
{
	.so_virt = (scan_virt_f)handle_virt
};

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;
	struct fragscan_info	info;

	printf("Welcome to fsl fragfind. Filesystem mode: \"%s\"\n", fsl_rt_fsname);

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		printf("Failed assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	DEBUG_TOOL_WRITE("Origin Type Now Allocated");

	info.virt_c = 0;
	scan_type(origin_ti, &ops, &info);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}
