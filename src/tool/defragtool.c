//#define DEBUG_TOOL
#include <inttypes.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "debug.h"
#include "runtime.h"
#include "type_info.h"
#include "choice.h"
#include "log.h"
#include "scan.h"
#include "writepkt.h"

#define RELNAME_DEFRAG	"defrag"

static struct choice_cache* ccache = NULL;
static int ccache_cursor = 0;

static bool is_fragmented(
	struct type_info* ti,
	const struct fsl_rtt_reloc* rel,
	int sel_min, int sel_max)
{
	struct type_info		*rel_sel_ti;
	diskoff_t			last_off, last_sz;
	int				sel_cur;

	if (sel_min >= sel_max) return false;

	sel_cur = sel_min;
	rel_sel_ti = typeinfo_follow_iter(ti, &rel->rel_sel, sel_cur);
	assert (rel_sel_ti != NULL);

	last_off = ti_phys_offset(rel_sel_ti);
	last_sz = ti_size(rel_sel_ti);

	typeinfo_free(rel_sel_ti);
	for (sel_cur = sel_cur+1; sel_cur <= sel_max; sel_cur++) {
		diskoff_t	cur_off, cur_sz;

		rel_sel_ti = typeinfo_follow_iter(ti, &rel->rel_sel, sel_cur);
		assert (rel_sel_ti != NULL);
		cur_off = ti_phys_offset(rel_sel_ti);
		cur_sz = ti_size(rel_sel_ti);
		typeinfo_free(rel_sel_ti);

		if (cur_off != (last_off + last_sz)) return true;
		last_off = cur_off;
		last_sz = cur_sz;
	}

	return false;
}

static void do_defrag(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	unsigned int			sel_min, sel_max;
	int				i, sel_count;
	int				cc_begin;

	if (ccache == NULL) {
		DEBUG_TOOL_WRITE("Allocating ccache.");
		ccache = choice_alloc(ti, rel);
		ccache_cursor = choice_min(ccache);
		DEBUG_TOOL_WRITE("Done allocating ccache.");
	}

	sel_min = rel->rel_sel.it_min(&ti_clo(ti));
	if (sel_min == ~0) return;

	sel_max = rel->rel_sel.it_max(&ti_clo(ti));
	if (sel_min > sel_max) return;

	sel_count = (sel_max - sel_min) + 1;

	/* no fragmentation if only one type instance */
	if (sel_count == 1) return;

	/* enough free space to relocate? */
	DEBUG_TOOL_WRITE("Verify free space: %d blocks", sel_count);
	cc_begin = choice_find_avail(ccache, ccache_cursor, sel_count);
	if (cc_begin == -1) {
		ccache_cursor = choice_min(ccache);
		cc_begin = choice_find_avail(ccache, ccache_cursor, sel_count);
		/* not enough space for full relocate? forget about it  */
		if (cc_begin == -1) return;
	}

	/* don't relocate if not actually fragmented */
	if (is_fragmented(ti, rel, sel_min, sel_max) == false) {
		DEBUG_TOOL_WRITE("OK. Not fragmented");
		return;
	}

	DEBUG_TOOL_WRITE("sel_count: %d", sel_count);
#ifdef DEBUG_TOOL
	typeinfo_print_path(ti); printf("\n");
#endif
	for (i = 0; i < sel_count; i++) {
		struct type_info	*ti_sel;
#ifdef DEBUG_TOOL
		DEBUG_TOOL_WRITE("Getting sel_idx=%d", sel_min+i);
		typeinfo_print_path(ti); printf("\n");
#endif
		ti_sel = typeinfo_follow_iter(ti, &rel->rel_sel, sel_min+i);
		assert (ti_sel != NULL);
		assert(choice_is_free(ccache, (cc_begin+i)));
		DEBUG_TOOL_WRITE("WPKT_RELOC: sel_idx=%d", sel_min+i);
		wpkt_relocate(ti, rel, ti_sel, sel_min+i, cc_begin+i);
		DEBUG_TOOL_WRITE("WPKT_RELOC DONE.");
		choice_mark_alloc(ccache, cc_begin+i);
		typeinfo_free(ti_sel);
	}

	ccache_cursor += sel_count;
	if (ccache_cursor > choice_max(ccache)) {
		ccache_cursor = choice_min(ccache);
	}
}

static int handle_ti(struct type_info* ti, void* aux)
{
	const struct fsl_rtt_type	*tt;
	int				i;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_reloc_c; i++) {
		const struct fsl_rtt_reloc	*rel;

		rel = &tt->tt_reloc[i];
		if (rel->rel_name == NULL) continue;
		if (strcmp(RELNAME_DEFRAG, rel->rel_name) != 0) continue;

		do_defrag(ti, rel);
	}

	return SCAN_RET_CONTINUE;
}

static struct scan_ops ops = { .so_ti = handle_ti };

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;

	printf("Welcome to fsl defrag. Filesystem mode: \"%s\"\n", fsl_rt_fsname);

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		printf("Failed assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	scan_type(origin_ti, &ops, NULL);

	if (ccache != NULL) choice_free(ccache);
	typeinfo_free(origin_ti);

	return 0;
}