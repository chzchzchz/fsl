#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>

#include "runtime.h"
#include "type_info.h"


#define pt_from_idx(x,y)	(&(tt_by_ti(x)->tt_pointsto[y]))

static void print_indent(unsigned int depth);
static void scan_type_pointsto(
	const struct type_info* ti,
	unsigned int pt_idx);
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
	TI_INTO_CLO			(ti);

	pt = pt_from_idx(ti, pt_idx);
	assert (pt->pt_range != NULL);

	tt = tt_by_num(pt->pt_type_dst);

	min_idx = pt->pt_min(clo);
	max_idx = pt->pt_max(clo);
	if (min_idx > max_idx) return;

	INDENT(ti);
	printf("points_to %s [%"PRIu64", %"PRIu64"]\n",
		(pt->pt_name) ? pt->pt_name : "", min_idx, max_idx);

	for (k = min_idx; k <= max_idx; k++) {
		struct type_info	*new_ti;
		struct type_desc	next_td;
		uint64_t		params[tt->tt_param_c];

		next_td.td_typenum = pt->pt_type_dst;

		td_init(&next_td,
			pt->pt_type_dst,
			pt->pt_range(clo, k, params),
			params);

		new_ti = typeinfo_alloc_pointsto(&next_td, pt_idx, k, ti);
		if (new_ti == NULL)
			continue;

		scan_type(new_ti);

		typeinfo_free(new_ti);
	}
}

static void scan_type_pointsto_all(const struct type_info* ti)
{
	const struct fsl_rt_table_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_pointsto_c; i++) {
		scan_type_pointsto(ti, i);
	}
}

static void dump_field(
	const struct fsl_rt_table_field* field,
	diskoff_t bitoff)
{
	printf("%s::", field->tf_fieldname);
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
	struct type_info*		new_ti;
	struct fsl_rt_table_field*	field;
	struct type_desc		next_td;
	uint64_t			num_elems;
	unsigned int			param_c;
	unsigned int			i;
	TI_INTO_CLO			(ti);

	field = &tt_by_ti(ti)->tt_field_thunkoff[field_idx];
	if (field->tf_typenum != TYPENUM_INVALID)
		param_c = tt_by_num(field->tf_typenum)->tt_param_c;
	else
		param_c = 0;
	uint64_t params[param_c];

	field->tf_params(clo, params);
	td_params(&next_td) = params;

	td_init(&next_td,
		field->tf_typenum,
		field->tf_fieldbitoff(clo),
		params);

	num_elems = field->tf_elemcount(clo);
	for (i = 0; i < num_elems; i++) {

		/* dump data */
		INDENT(ti);
		dump_field(field, td_offset(&next_td));
		if (next_td.td_typenum == TYPENUM_INVALID)
			return;

		/* recurse */
		new_ti = typeinfo_alloc(&next_td, field_idx, ti);
		if (i < num_elems - 1) {
			size_t	sz;
			/* move to next element */
			TD_INTO_CLO_DECL(new_clo, &next_td);
			sz = tt_by_num(field->tf_typenum)->tt_size(&new_clo);
			td_offset(&next_td) += sz;
		}

		if (new_ti == NULL) {
			continue;
		}

		INDENT(ti);
		printf("following field %s\n", field->tf_fieldname);

		scan_type(new_ti);


		typeinfo_free(new_ti);
	}
}

static void handle_virt(struct type_info* ti, struct fsl_rt_table_virt* vt)
{
	unsigned int			i;
	int				err_code;

	err_code = TI_ERR_OK;
	i = 0;
	while (err_code == TI_ERR_OK || err_code == TI_ERR_BADIDX) {
		struct type_info		*ti_cur;

		ti_cur = typeinfo_alloc_virt_idx(vt, ti, i, &err_code);
		if (ti_cur == NULL) {
			i++;
			printf("next\n");
			continue;
		}

		INDENT(ti);
		if (vt->vt_name != NULL) printf("virt %s (%d)\n", vt->vt_name, i);
		else printf("virt (%d)\n", i);

		assert (ti_cur->ti_td.td_clo.clo_xlate != NULL);

		scan_type(ti_cur);

		typeinfo_free(ti_cur);

		i++;
	}
}


static void scan_type_virt(struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_virt_c; i++) {
		handle_virt(ti, &tt->tt_virt[i]);
	}
}

/* dump all data for strong usertypes that are aggregate to the given type */
static void scan_type_strongtypes(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_field_c; i++) {
		handle_field(ti, i);
	}
}

static void scan_type(struct type_info* ti)
{
	unsigned int i;

	INDENT(ti);
	printf("scanning: %s (%d usertypes) voff=%"PRIu64" bits. poff=%"PRIu64" bits. xlate=%p\n",
		tt_by_ti(ti)->tt_name,
		tt_by_ti(ti)->tt_field_c,
		ti_offset(ti),
		ti_phys_offset(ti),
		ti_xlate(ti));
	scan_type_strongtypes(ti);
	scan_type_pointsto_all(ti);
	scan_type_virt(ti);
}

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;
	struct type_desc	init_td = td_origin();
	unsigned int 		i;

	printf("Welcome to fsl scantool. Scan mode: \"%s\"\n", fsl_rt_fsname);

	origin_ti = typeinfo_alloc(&init_td, 0, NULL);
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		return -1;
	}

	scan_type(origin_ti);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}
