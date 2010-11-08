//#define DEBUG_TOOL
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include "runtime.h"
#include "debug.h"
#include "type_info.h"

#define pt_from_idx(x,y)	(&(tt_by_ti(x)->tt_pointsto[y]))

static void print_indent(unsigned int depth);
static void scan_type_pointsto(const struct type_info* ti, unsigned int pt_idx);
static void scan_type_pointsto_all(const struct type_info* ti);
static void scan_type_strongtypes(const struct type_info* ti);
static void scan_type_virt(struct type_info* ti);
static void scan_type(struct type_info* ti);

#define INDENT(x)	print_indent(ti_depth(x))

static void print_indent(unsigned int depth)
{
	while (depth) {
		printf(" ");
		depth--;
	}
}

static void scan_type_pointsto(
	const struct type_info* ti,
	unsigned int pt_idx)
{
	struct fsl_rt_table_pointsto	*pt;
	struct fsl_rt_table_type	*tt;
	uint64_t			k;
	uint64_t			min_idx, max_idx;

	DEBUG_TOOL_ENTER();
	FSL_DYN_SAVE(dyn_saved);

	pt = pt_from_idx(ti, pt_idx);
	assert (pt->pt_range != NULL);

	tt = tt_by_num(pt->pt_type_dst);

	min_idx = pt->pt_min(&ti_clo(ti));
	max_idx = pt->pt_max(&ti_clo(ti));
	if (min_idx > max_idx) {
		FSL_DYN_RESTORE(dyn_saved);
		DEBUG_TOOL_LEAVE();
		return;
	}

	DEBUG_TOOL_WRITE(
		"points_to %s %s [%"PRIu64", %"PRIu64"]",
		(pt->pt_name) ? pt->pt_name : "",
		tt->tt_name,
		min_idx, max_idx);

	for (k = min_idx; k <= max_idx; k++) {
		struct type_info	*new_ti;

		DEBUG_TOOL_WRITE("allocating pt[%d]", k);

		FSL_DYN_LOAD(dyn_saved);
		new_ti = typeinfo_follow_pointsto(ti, pt, k);
		if (new_ti == NULL) {
			DEBUG_TOOL_WRITE("skipped");
			continue;
		}else {
			DEBUG_TOOL_WRITE("found pointsto");
		}

		DEBUG_TOOL_WRITE("Followng points-to");
		scan_type(new_ti);

		typeinfo_free(new_ti);
	}

	FSL_DYN_RESTORE(dyn_saved);
	DEBUG_TOOL_LEAVE();
}

static void scan_type_pointsto_all(const struct type_info* ti)
{
	const struct fsl_rt_table_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_pointsto_c; i++)
		scan_type_pointsto(ti, i);
}

static void dump_field(
	const struct fsl_rt_table_field* field,
	diskoff_t bitoff,
	int idx)
{
	if (idx < 0)	printf("%s::", field->tf_fieldname);
	else		printf("%s[%d]::", field->tf_fieldname, idx);

	printf("%s",
		(field->tf_typenum != TYPENUM_INVALID) ?
			tt_by_num(field->tf_typenum)->tt_name :
			"ANONYMOUS");

	printf("@offset=0x%" PRIx64 " (%" PRIu64 ")\n", bitoff, bitoff);
}

static void handle_field(
	const struct type_info* ti,
	unsigned int field_idx)
{
	struct fsl_rt_table_field*	field;
	uint64_t			num_elems;
	unsigned int			i;
	size_t				off;

	if (ti_typenum(ti) == TYPENUM_INVALID) return;

	field = &tt_by_ti(ti)->tt_fieldstrong_table[field_idx];
	num_elems = field->tf_elemcount(&ti_clo(ti));
	off = field->tf_fieldbitoff(&ti_clo(ti));

	for (i = 0; i < num_elems; i++) {
		struct type_info*		new_ti;

		/* dump data */
		INDENT(ti);

		if (num_elems == 1) dump_field(field, off, -1);
		else dump_field(field, off, i);

		new_ti = typeinfo_follow_field_off(ti, field, off);
		if (new_ti == NULL) continue;

		/* recurse */
		if (i < num_elems - 1) {
			/* move to next element */
			if (ti_typenum(new_ti) == TYPENUM_INVALID)
				off += field->tf_typesize(&ti_clo(ti));
			else
				off += tt_by_ti(new_ti)->tt_size(&ti_clo(new_ti));
		}

		DEBUG_TOOL_WRITE("following field %s", field->tf_fieldname);

		scan_type(new_ti);

		typeinfo_free(new_ti);
	}
}

static void handle_virt(struct type_info* ti, struct fsl_rt_table_virt* vt)
{
	unsigned int			i;
	int				err_code;
	struct type_info		*ti_base;

	DEBUG_TOOL_ENTER();

	FSL_DYN_SAVE(dyn_saved);
	err_code = TI_ERR_OK;
	i = 0;
	while (err_code == TI_ERR_OK || err_code == TI_ERR_BADVERIFY) {
		struct type_info		*ti_cur;

		DEBUG_TOOL_WRITE("alloc virt %s[%d]", vt->vt_name, i);
		FSL_DYN_LOAD(dyn_saved);
		ti_cur = typeinfo_follow_virt(ti, vt, i, &err_code);
		if (ti_cur == NULL) {
			DEBUG_TOOL_WRITE(
				"handle_virt: (skipping %s %d err=%d)",
				vt->vt_name, i, err_code);
			i++;
			continue;
		}

		if (vt->vt_name != NULL) DEBUG_TOOL_WRITE("virt initialized! %s (%d)", vt->vt_name, i);
		else DEBUG_TOOL_WRITE("virt initialized! (%d)", i);
		DEBUG_TOOL_WRITE("voff=%"PRIu64, ti_offset(ti_cur));

		assert (ti_cur->ti_td.td_clo.clo_xlate != NULL);

		DEBUG_TOOL_WRITE("Following virtual %s[%d]", vt->vt_name, i);
		scan_type(ti_cur);

		DEBUG_TOOL_WRITE("virt done");
		typeinfo_free(ti_cur);

		DEBUG_TOOL_WRITE("free virt %s[%d]", vt->vt_name, i);
		i++;
	}

	FSL_DYN_RESTORE(dyn_saved);
	DEBUG_TOOL_LEAVE();
}

static void scan_type_virt(struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	DEBUG_TOOL_ENTER();
	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_virt_c; i++) {
		handle_virt(ti, &tt->tt_virt[i]);
	}
	DEBUG_TOOL_LEAVE();
}

/* dump all data for strong usertypes that are aggregate to the given type */
static void scan_type_strongtypes(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_fieldstrong_c; i++) {
		handle_field(ti, i);
	}
}

static void scan_type(struct type_info* ti)
{
	unsigned int	i;
	typesize_t	size;
	voff_t		voff;
	poff_t		poff;

	if (ti_typenum(ti) == TYPENUM_INVALID) return;

	DEBUG_TOOL_ENTER();

	voff = ti_offset(ti);
	poff = ti_phys_offset(ti);
	size = ti_size(ti);

	DEBUG_TOOL_WRITE("poff=%"PRIu64, poff);
	assert (offset_in_range(poff) && "NOT ON DISK?");
#if 0
	printf("scanning: %s (%d usertypes) voff=%"PRIu64
			" bits. poff=%"PRIu64
			" bits. xlate=%p\n",
		tt_by_ti(ti)->tt_name,
		tt_by_ti(ti)->tt_fieldstrong_c,
		voff,
		poff,
		ti_xlate(ti));
#endif
#ifdef DEBUG_TOOL
	typeinfo_print_path(ti);
	printf("\n");
#endif

	printf("{ 'Mode' : 'Scan', 'voff' : %"PRIu64
		", 'poff' : %"PRIu64
		", 'size': %"PRIu64
		", 'name' : '%s' }\n",
		voff, poff, size,
		tt_by_ti(ti)->tt_name);
	scan_type_strongtypes(ti);
	DEBUG_TOOL_WRITE("do pointsto_all.");
	scan_type_pointsto_all(ti);
	DEBUG_TOOL_WRITE("do scan virts.");
	scan_type_virt(ti);

	DEBUG_TOOL_LEAVE();
}

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;
	struct type_desc	init_td = td_origin();
	unsigned int 		i;

	printf("Welcome to fsl scantool. Scan mode: \"%s\"\n", fsl_rt_fsname);

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_by_field(&init_td, NULL, NULL);
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		return -1;
	}

	DEBUG_TOOL_WRITE("Origin Type Now Allocated");

	scan_type(origin_ti);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}
