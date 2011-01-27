/* try to smush as close to the top as possible */
//#define DEBUG_TOOL
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "debug.h"
#include "type_info.h"
#include "bitmap.h"
#include "scan.h"
#include "log.h"
#include "choice.h"
#include "writepkt.h"

#define RELNAME_RELOC	"reloc"

struct scatterscan_info { unsigned int	rel_c; };

static struct choice_cache	*ccache = NULL;
static int			refresh_count = 0;
static int64_t			ccache_cursor = 0;
#define MAX_REFRESH		40

static void cache_refresh(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	choice_free(ccache);
	ccache = choice_alloc(ti, rel);
	ccache_cursor = choice_max(ccache);
	refresh_count++;
}

/*
 * get a free block to replace
 */
uint64_t choice_find(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	uint64_t	ccmin;

	if (refresh_count == MAX_REFRESH) return ~0;

	ccmin = choice_min(ccache);
	for (; ccache_cursor >= ccmin; ccache_cursor--) {
		if (choice_is_alloc(ccache, ccache_cursor)) continue;
		assert (choice_is_free(ccache, ccache_cursor));
		return ccache_cursor;
	}

	cache_refresh(ti, rel);

	return ~0;
}

/* compare location of selected with reloc locations */
/* if reloc location needs to be set, try to swap in */
/* if rel_sel location is on empty reloc location, try to swap out */
void swap_rel_sel(
	struct type_info* ti, const struct fsl_rtt_reloc* rel,
	struct type_info* rel_sel_ti, unsigned int sel_v)
{
	struct type_info*	replace_choice_ti;
	uint64_t		replace_choice_idx;
	poff_t			replace_poff, rel_sel_poff;

	DEBUG_TOOL_ENTER();

	/* need to move this to an unused location that
	 * is marked in the image */
	replace_choice_idx = choice_find(ti, rel);
	if (replace_choice_idx == ~0) {
		DEBUG_TOOL_WRITE("No more moves left. Oh well.\n");
		goto done;
	}
	DEBUG_TOOL_WRITE("exchanging: sel_v=%d with choice_v=%"PRIu64,
		sel_v, replace_choice_idx);

	replace_choice_ti = typeinfo_follow_iter(
		ti, &rel->rel_choice, replace_choice_idx);
	if (replace_choice_ti == NULL) {
		DEBUG_TOOL_WRITE(
			"Couldn't allocate replace choice idx=%"PRIu64,
			replace_choice_idx);
		goto done;
	}

	/* always more toward end of disk */
	replace_poff = ti_phys_offset(replace_choice_ti);
	rel_sel_poff = ti_phys_offset(rel_sel_ti);
	typeinfo_free(replace_choice_ti);
	if (rel_sel_poff >= replace_poff) {
		DEBUG_TOOL_WRITE("ignoring reloc old=%"PRIu64" -> new=%"PRIu64,
			rel_sel_poff, replace_poff);
		goto done;
	}

	wpkt_relocate(ti, rel, rel_sel_ti, sel_v, replace_choice_idx);
	choice_mark_alloc(ccache, replace_choice_idx);
	assert (choice_is_alloc(ccache, replace_choice_idx));
	assert (!choice_is_free(ccache, replace_choice_idx));
done:
	DEBUG_TOOL_LEAVE();
}

void do_rel_type(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	const struct fsl_rtt_type	*dst_type;
	int				sel_cur, sel_min, sel_max;

	if (ccache == NULL) {
		ccache = choice_alloc(ti, rel);
//		choice_dump(ccache);
//		exit(1);
		ccache_cursor = choice_max(ccache);
	}

	dst_type = tt_by_num(rel->rel_sel.it_type_dst);
	sel_min = rel->rel_sel.it_min(&ti_clo(ti));
	sel_max = rel->rel_sel.it_max(&ti_clo(ti));

	DEBUG_TOOL_WRITE(
		"DO_REL_TYPE voff=%"PRIu64
		" poff=%"PRIu64" sel_min=%d // sel_max=%d",
		ti_offset(ti), ti_phys_offset(ti),
		sel_min, sel_max);

	for (sel_cur = sel_min; sel_cur <= sel_max; sel_cur++) {
		struct type_info		*rel_sel_ti;

		rel_sel_ti = typeinfo_follow_iter(ti, &rel->rel_sel, sel_cur);
		if (rel_sel_ti == NULL) {
			DEBUG_TOOL_WRITE("Failed to get rel_sel_ti for idx=%d\n",
				sel_cur);
			continue;
		}
		swap_rel_sel(ti, rel, rel_sel_ti, sel_cur);
		typeinfo_free(rel_sel_ti);
	}
}

int ti_handle(struct type_info* ti, struct scatterscan_info* rinfo)
{
	const struct fsl_rtt_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	if (tt->tt_reloc_c == 0)
		return SCAN_RET_CONTINUE;

	for (i = 0; i < tt->tt_reloc_c; i++) {
		const struct fsl_rtt_reloc	*rel = &tt->tt_reloc[i];
		if (rel->rel_name == NULL) continue;
		if (strcmp(rel->rel_name, RELNAME_RELOC) != 0) continue;
		do_rel_type(ti, rel);
	}

	rinfo->rel_c++;

	return SCAN_RET_CONTINUE;
}

struct scan_ops ops = { .so_ti = (scan_ti_f)ti_handle };

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;
	struct scatterscan_info	info;

	printf("Welcome to fsl thrashcopy. Filesystem mode: \"%s\"\n", fsl_rt_fsname);
	assert (argc == 0 && "./thrashcopy hd.img");

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		printf("Failed assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	DEBUG_TOOL_WRITE("Origin Type Now Allocated");

	info.rel_c = 0;
	scan_type(origin_ti, &ops, &info);

	if (ccache != NULL) choice_free(ccache);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}