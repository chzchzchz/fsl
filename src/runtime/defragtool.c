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

	for (sel_cur = sel_cur+1; sel_cur <= sel_max; sel_cur++) {
		diskoff_t	cur_off, cur_sz;
		rel_sel_ti = typeinfo_follow_iter(ti, &rel->rel_sel, sel_cur);
		assert (rel_sel_ti != NULL);
		cur_off = ti_phys_offset(rel_sel_ti);
		cur_sz = ti_size(rel_sel_ti);
		if (cur_off != (last_off + last_sz)) return true;
		last_off = cur_off;
		last_sz = cur_sz;
		typeinfo_free(rel_sel_ti);
	}

	return false;
}

/* TODO: PUT THIS IN ITS OWN FILE */
static void wpkt_relocate(
	struct type_info* ti_parent,
	const struct fsl_rtt_reloc* rel,
	struct type_info* rel_sel_ti,
	unsigned int sel_idx,
	unsigned int choice_idx)
{
	struct type_info	*replace_choice_ti;
	struct fsl_rt_wlog	wlog_replace;
	uint64_t		wpi_params[2];
	uint64_t		wpkt_params[16]; /* XXX: this should be max of wpkts */

	replace_choice_ti = typeinfo_follow_iter(
		ti_parent, &rel->rel_choice, choice_idx);
	assert (replace_choice_ti != NULL);

	/* wpkt_alloc => (choice_idx) */
	FSL_WRITE_START();
	wpi_params[0] = choice_idx;
	rel->rel_alloc.wpi_params(&ti_clo(ti_parent), wpi_params, wpkt_params);
	fsl_io_do_wpkt(rel->rel_alloc.wpi_wpkt, wpkt_params);
#ifdef DEBUG_TOOL
	printf("ALLOC PENDING: \n");
	fsl_io_dump_pending();
#endif
	FSL_WRITE_COMPLETE();

	typeinfo_phys_copy(replace_choice_ti, rel_sel_ti);
#ifdef DEBUG_TOOL
	printf("COPIED: %"PRIu64" -> %"PRIu64"\n",
		ti_phys_offset(rel_sel_ti) / 8,
		ti_phys_offset(replace_choice_ti) / 8);

	printf("rel_sel: ");
	typeinfo_print_path(rel_sel_ti);
	printf("\n");
	printf("replace_ti: ");
	typeinfo_print_path(replace_choice_ti);
	printf("\n");
#endif

	/* wpkt_replace => (sel_idx) */
	FSL_WRITE_START();
	wpi_params[0] = sel_idx;
	rel->rel_replace.wpi_params(&ti_clo(ti_parent), wpi_params, wpkt_params);
	fsl_io_do_wpkt(rel->rel_replace.wpi_wpkt, wpkt_params);
	DEBUG_TOOL_WRITE("REPLACE PENDING:");
#ifdef DEBUG_TOOL
	fsl_io_dump_pending();
#endif

	fsl_io_steal_wlog(fsl_get_io(), &wlog_replace);

	/* wpkt_relink => (sel_idx, choice_idx) */
	FSL_WRITE_START();
	wpi_params[0] = sel_idx;
	wpi_params[1] = choice_idx;
	rel->rel_relink.wpi_params(&ti_clo(ti_parent), wpi_params, wpkt_params);
	fsl_io_do_wpkt(rel->rel_relink.wpi_wpkt, wpkt_params);

	DEBUG_TOOL_WRITE("RELINK PENDING:");
#ifdef DEBUG_TOOL
	fsl_io_dump_pending();
#endif
	FSL_WRITE_COMPLETE();

	fsl_wlog_commit(&wlog_replace);

	typeinfo_free(replace_choice_ti);
}

static void do_defrag(
	struct type_info* ti, const struct fsl_rtt_reloc* rel)
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
	sel_max = rel->rel_sel.it_max(&ti_clo(ti));
	if (sel_min > sel_max) return;

	sel_count = (sel_max - sel_min) + 1;

	/* no fragmentation if only one type instance */
	if (sel_count == 1) return;

	/* enough free space to relocate? */
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

	for (i = 0; i < sel_count; i++) {
		struct type_info	*ti_sel;
		ti_sel = typeinfo_follow_iter(ti, &rel->rel_sel, sel_min+i);
		assert (ti_sel != NULL);
		assert(choice_is_set(ccache, (cc_begin+i)-choice_min(ccache)));
		wpkt_relocate(ti, rel, ti_sel, sel_min+i, cc_begin+i);
		choice_unset(ccache, (cc_begin+i)-choice_min(ccache));
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
	struct type_desc	init_td = td_origin();

	printf("Welcome to fsl defrag. Filesystem mode: \"%s\"\n", fsl_rt_fsname);

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_by_field(&init_td, NULL, NULL);

	scan_type(origin_ti, &ops, NULL);

	if (ccache != NULL) choice_free(ccache);
	typeinfo_free(origin_ti);

	return 0;
}