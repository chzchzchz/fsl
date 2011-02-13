//#define DEBUG_TOOL
//#define NO_WRITES
#include <inttypes.h>
#include "runtime.h"
#include "debug.h"
#include "type_info.h"
#include "bitmap.h"
#include "scan.h"
#include "io.h"
#include "log.h"
#include "choice.h"
#include "writepkt.h"

/* relocation procedure:
 * 1. allocate
 * 2. copy rel_sel_ti into newly allocated
 * 2. relink newly allocated into rel_sel_ti reference
 * 3. replace rel_sel_ti entry into free pool
 *
 * NOTE: relink and replace rely on *initial values* before reloc
 * therefore it is imperative to keep wlogs for both and only
 * commit after both have been computed
 *
 * Alloc must be committed before replace and relink. This ensures
 * that things like free counts are properly reflected in replace.
 */
void wpkt_relocate(
	struct type_info* ti_parent,
	const struct fsl_rtt_reloc* rel,
	struct type_info* rel_sel_ti,
	unsigned int sel_idx,
	unsigned int choice_idx)
{
	struct type_info	*replace_choice_ti;
	struct fsl_rt_wlog	wlog_replace;
	uint64_t		wpi_params[2];
	uint64_t		wpkt_params[16]; /* XXX: should be max of wpkts */

	replace_choice_ti = typeinfo_follow_iter(
		ti_parent, &rel->rel_choice, choice_idx);
	if (replace_choice_ti == NULL) {
		DEBUG_WRITE("WANTED REPLACE CHOICE %"PRIu32"\n", choice_idx);
		DEBUG_WRITE("BUT COULDN'T GET IT??\n");
	}
	FSL_ASSERT (replace_choice_ti != NULL);

	/* wpkt_alloc => (choice_idx) */
	FSL_WRITE_START();
	wpi_params[0] = choice_idx;
	rel->rel_alloc.wpi_params(&ti_clo(ti_parent), wpi_params, wpkt_params);
	fsl_io_do_wpkt(rel->rel_alloc.wpi_wpkt, wpkt_params);
#ifdef DEBUG_TOOL
	DEBUG_WRITE("ALLOC PENDING:");
	fsl_io_dump_pending();
#endif
#ifdef NO_WRITES
	FSL_WRITE_DROP();
#else
	DEBUG_TOOL_WRITE("COMPLETINGING ALLOC");
	FSL_WRITE_COMPLETE();
#endif

	DEBUG_TOOL_WRITE("ALLOC COMPLETE!");
#ifndef NO_WRITES
	typeinfo_phys_copy(replace_choice_ti, rel_sel_ti);
#endif
	DEBUG_TOOL_WRITE("PHYS COPY COMPLETE!");

#ifdef DEBUG_TOOL
	DEBUG_WRITE("COPIED: %"PRIu64" -> %"PRIu64"\n",
		ti_phys_offset(rel_sel_ti) / 8,
		ti_phys_offset(replace_choice_ti) / 8);

	DEBUG_WRITE("rel_sel: ");
	typeinfo_print_path(rel_sel_ti);
	DEBUG_WRITE("\n");
	DEBUG_WRITE("replace_ti: ");
	typeinfo_print_path(replace_choice_ti);
	DEBUG_WRITE("\n");
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
	DEBUG_TOOL_WRITE("STEALING LOG");
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
#ifdef NO_WRITES
	FSL_WRITE_DROP();
#else
	DEBUG_TOOL_WRITE("COMPLETE WRITE.");
	FSL_WRITE_COMPLETE();
	DEBUG_TOOL_WRITE("COMMIT LOG.");
	fsl_wlog_commit(&wlog_replace);
#endif
	DEBUG_TOOL_WRITE("FREE REPLACE CHOICE");
	typeinfo_free(replace_choice_ti);
}
