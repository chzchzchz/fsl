/* try to smush as close to the top as possible */
//#define DEBUG_TOOL
#include <stdint.h>
#include <string.h>
#include "runtime.h"
#include "tool.h"
#include "info.h"
#include "debug.h"
#include "type_info.h"
#include "scan.h"
#include "log.h"
#include "choice.h"
#include "writepkt.h"

#define RELNAME_RELOC	"reloc"

struct scatterscan_info { unsigned int	rel_c; };

static struct choice_cache	*ccache = NULL;
static int			refresh_count = 0;
static uint64_t			ccache_cursor = 0;
static uint64_t			ccache_cursor_threshold = 0;
#define MAX_REFRESH		10

static void cache_refresh(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	choice_free(ccache);
	ccache = choice_alloc(ti, rel);
	ccache_cursor = choice_min(ccache);
	refresh_count++;
}

/* get a free block to replace */
static uint64_t choice_find(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	uint64_t	ccmax;

	ccmax = choice_max(ccache);
	for (; ccache_cursor <= ccmax; ccache_cursor++) {
		int		k;

		k = choice_find_avail(ccache, ccache_cursor, 1);
		if (k == -1) continue;
		/* check cache first. not set => not available */
		if (k > ccache_cursor_threshold) {
			cache_refresh(ti, rel);
			ccache_cursor_threshold += ccache_cursor_threshold/2;
			ccache_cursor -= (ccache_cursor-choice_min(ccache))/2;
			continue;
		}
		if (choice_is_alloc(ccache, k)) continue;
		ccache_cursor = k+1;

		return k;
	}

	if (refresh_count == MAX_REFRESH) return ~0;
	cache_refresh(ti, rel);

	return ~0;
}

/* compare location of selected with reloc locations */
/* if reloc location needs to be set, try to swap in */
/* if rel_sel location is on empty reloc location, try to swap out */
static void swap_rel_sel(
	struct type_info* ti, const struct fsl_rtt_reloc* rel,
	struct type_info* rel_sel_ti, unsigned int sel_v)
{
	struct type_info*	replace_choice_ti;
	uint64_t		replace_choice_idx;
	diskoff_t		replace_poff, rel_sel_poff;

	/* already in an OK place, don't do anything
	 * (in future, may want to steal if we have overlaps) */
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
		DEBUG_TOOL_WRITE("Couldn't allocate replace choice");
		goto done;
	}

	replace_poff = ti_phys_offset(replace_choice_ti);
	rel_sel_poff = ti_phys_offset(rel_sel_ti);
	typeinfo_free(replace_choice_ti);

	if (replace_poff > rel_sel_poff) {
		DEBUG_TOOL_WRITE("Do not replace if choice is > victim");
		ccache_cursor = replace_choice_idx;
		goto done;
	}

	wpkt_relocate(ti, rel, rel_sel_ti, sel_v, replace_choice_idx);
	choice_mark_alloc(ccache, replace_choice_idx);
done:
	DEBUG_TOOL_LEAVE();
}

static void do_rel_type(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	const struct fsl_rtt_type	*dst_type;
	int				sel_cur, sel_min, sel_max;

	if (ccache == NULL) {
		ccache = choice_alloc(ti, rel);
		ccache_cursor = choice_min(ccache);
		ccache_cursor_threshold = (choice_max(ccache)+choice_min(ccache))/2;
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

static int ti_handle(struct type_info* ti, struct scatterscan_info* rinfo)
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

static struct scan_ops ops = { .so_ti = (scan_ti_f)ti_handle };

TOOL_ENTRY(smush)
{
	struct type_info	*origin_ti;
	struct scatterscan_info	info;

	FSL_INFO("Welcome to fsl smushtool. Filesystem mode: \"%s\"\n", fsl_rt_fsname);
	FSL_ASSERT (argc == 0 && "./smushtool hd.img");

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		FSL_INFO("Could not open origin type\n");
		FSL_INFO("Failed assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	DEBUG_TOOL_WRITE("Origin Type Now Allocated");

	info.rel_c = 0;
	scan_type(origin_ti, &ops, &info);

	if (ccache != NULL) choice_free(ccache);

	typeinfo_free(origin_ti);

	FSL_INFO("fslsmush: Have a nice day\n");
	return 0;
}