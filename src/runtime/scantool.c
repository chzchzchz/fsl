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
static void scan_type(const struct type_info* ti);

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
	unsigned int			k;
	unsigned int			min_idx, max_idx;

	pt = pt_from_idx(ti, pt_idx);
	assert (pt->pt_range != NULL);

	tt = tt_by_num(pt->pt_type_dst);

	min_idx = pt->pt_min(ti->ti_diskoff, ti->ti_params);
	max_idx = pt->pt_max(ti->ti_diskoff, ti->ti_params);
	if (min_idx != max_idx)
		printf("[%d,%d]\n", min_idx, max_idx);
	for (k = min_idx; k <= max_idx; k++) {
		struct type_info	*new_ti;
		uint64_t		params[tt->tt_param_c];

		new_ti = typeinfo_alloc_pointsto(
			pt->pt_type_dst,
			pt->pt_range(ti->ti_diskoff, ti->ti_params, k, params),
			params,
			pt_idx,
			k,
			ti);
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
		(field->tf_typenum != ~0) ?
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
	uint64_t			bitoff;
	uint64_t			num_elems;
	unsigned int			param_c;
	unsigned int			i;

	field = &tt_by_ti(ti)->tt_field_thunkoff[field_idx];
	bitoff = field->tf_fieldbitoff(ti->ti_diskoff, ti->ti_params);
	num_elems = field->tf_elemcount(ti->ti_diskoff, ti->ti_params);

	if (field->tf_typenum != ~0)
		param_c = tt_by_num(field->tf_typenum)->tt_param_c;
	else
		param_c = 0;
	uint64_t params[param_c];

	field->tf_params(ti->ti_diskoff, ti->ti_params, params);

	for (i = 0; i < num_elems; i++) {
		/* dump data */
		print_indent(typeinfo_get_depth(ti));
		dump_field(field, bitoff);
		if (field->tf_typenum == ~0)
			return;

		/* recurse */
		new_ti = typeinfo_alloc(
			field->tf_typenum, bitoff, params, field_idx, ti);
		if (new_ti == NULL)
			continue;

		scan_type(new_ti);

		if (i < num_elems - 1) {
			/* more to next element */
			bitoff += tt_by_ti(new_ti)->tt_size(
				new_ti->ti_diskoff, new_ti->ti_params);
		}

		typeinfo_free(new_ti);
	}
}

/* dump all data for strong usertypes that are aggregate to the given type */
static void scan_type_strong_types(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_field_c; i++) {
		handle_field(ti, i);
	}
}

static void scan_type(const struct type_info* ti)
{
	unsigned int i;

	print_indent(typeinfo_get_depth(ti));
	printf("scanning: %s (%d usertypes)\n",
		tt_by_ti(ti)->tt_name,
		tt_by_ti(ti)->tt_field_c);
	scan_type_strong_types(ti);
	scan_type_pointsto_all(ti);
}

void tool_entry(void)
{
	struct type_info	*origin_ti;
	unsigned int 		i;

	printf("Welcome to fsl scantool. Scan mode: \"%s\"\n", fsl_rt_fsname);

	origin_ti = typeinfo_alloc(fsl_rt_origin_typenum, 0, NULL, 0, NULL);
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		return;
	}

	typeinfo_set_depth(origin_ti, 1);

	scan_type(origin_ti);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
}
