/* scatter the data to the windssssss */
//#define DEBUG_TOOL
#include <inttypes.h>
#include <string.h>
#include "runtime.h"
#include "debug.h"
#include "info.h"
#include "tool.h"
#include "type_info.h"
#include "scan.h"
#include "log.h"
#include "choice.h"
#include "writepkt.h"

/* OK, these relname things are getting out of hand,
 * we need some abstraction to let the user / program
 * control it. */
#define RELNAME_RELOC	"reloc"

struct scatterscan_info { unsigned int	rel_c; };

static struct choice_cache	*ccache = NULL;
static int			refresh_count = 0;
static int			frag_percent = 100;
static int			num_relocs = 0;
#define MAX_REFRESH		10

/*
 * get a random free block to replace
 */
static uint64_t choice_find(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	int	i;
	for (i = 0; i <= 10; i++) {
		uint64_t	rand_choice;
		uint64_t	choice_base;
		int		k;

		rand_choice = FSL_RAND() % (choice_max(ccache) - choice_min(ccache));
		choice_base = rand_choice + choice_min(ccache);
		k = choice_find_avail(ccache, choice_base, 1);
		if (k == -1) continue;
		/* check cache first. not set => not available */
		if (choice_is_alloc(ccache, k)) continue;
		FSL_ASSERT (k <= choice_max(ccache) && "Out of bounds choice.");
		return k;
	}

	if (refresh_count == MAX_REFRESH) return ~0;

	choice_free(ccache);
	ccache = choice_alloc(ti, rel);
	refresh_count++;

	return ~0;
}

/* compare location of selected with reloc locations */
/* if reloc location needs to be set, try to swap in */
/* if rel_sel location is on empty reloc location, try to swap out */
static void swap_rel_sel(
	struct type_info* ti, const struct fsl_rtt_reloc* rel,
	struct type_info* rel_sel_ti, unsigned int sel_v)
{
	uint64_t		replace_choice_idx;

	/* limit fragmentation to user defined percentage */
	if ((FSL_RAND() % 100) > frag_percent) return;

	/* already in an OK place, don't do anything
	 * (in future, may want to steal if we have overlaps) */
	DEBUG_TOOL_ENTER();

	/* need to move this to an unused location that
	 * is marked in the image */
	replace_choice_idx = choice_find(ti, rel);
	if (replace_choice_idx == ~0) {
		DEBUG_TOOL_WRITE("No more moves left. Oh well.\n");
		return;
	}
	DEBUG_TOOL_WRITE("exchanging: sel_v=%d with choice_v=%"PRIu64,
		sel_v, replace_choice_idx);

	wpkt_relocate(ti, rel, rel_sel_ti, sel_v, replace_choice_idx);
	num_relocs++;
	if ((num_relocs % 100) == 0) {
		FSL_INFO("NUM RELOCS: %d\r", num_relocs);
	}
	choice_mark_alloc(ccache, replace_choice_idx);

	DEBUG_TOOL_LEAVE();
}

static void do_rel_type(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	const struct fsl_rtt_type	*dst_type;
	int				sel_cur, sel_min, sel_max;

	if (ccache == NULL) ccache = choice_alloc(ti, rel);

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
	if (tt->tt_reloc_c == 0) return SCAN_RET_CONTINUE;

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

TOOL_ENTRY(scatter)
{
	struct type_info	*origin_ti;
	struct scatterscan_info	info;

	FSL_INFO("Welcome to fsl scatter. Filesystem mode: \"%s\"\n", fsl_rt_fsname);
	FSL_ASSERT (argc >= 0 && "./scattertool hd.img [fragpercent]");
	if (argc > 0) frag_percent = FSL_ATOI(argv[0]);

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		FSL_INFO("Could not open origin type\n");
		FSL_INFO("Failed assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	DEBUG_TOOL_WRITE("Origin Type Now Allocated");

	FSL_INFO("Scattering with fragmentation percentage %d%%\n", frag_percent);

	info.rel_c = 0;
	scan_type(origin_ti, &ops, &info);
	FSL_INFO("\nDone relocating.\n");
	if (ccache != NULL) choice_free(ccache);

	typeinfo_free(origin_ti);

	FSL_INFO("Have a nice day\n");
	return 0;
}
