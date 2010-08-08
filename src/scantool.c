#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include "runtime.h" 

#define tt_by_num(x)	(&fsl_rt_table[x])
#define tt_by_ti(x)	tt_by_num((x)->ti_typenum)

struct type_info
{
	typenum_t	ti_typenum;
	diskoff_t	ti_diskoff;
	unsigned int	ti_depth;
};

static void print_indent(unsigned int depth);
static void scan_type_pointsto(
	const struct type_info* ti, 
	const struct fsl_rt_table_pointsto* pt);
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
	const struct fsl_rt_table_pointsto* pt)
{
	struct type_info	new_ti;
	unsigned int		k;
	unsigned int		min_idx, max_idx;

	new_ti.ti_typenum = pt->pt_type_dst;
	new_ti.ti_depth = ti->ti_depth + 1;

	if (pt->pt_single != NULL) {
		new_ti.ti_diskoff = pt->pt_single(ti->ti_diskoff);
		print_indent(ti->ti_depth);
		printf("points-to-single@%"PRIx64" -> %"PRId64 " (%s)\n", 
			ti->ti_diskoff,
			new_ti.ti_diskoff,
			fsl_rt_table[new_ti.ti_typenum].tt_name);
		scan_type(&new_ti);
		return;
	} 

	assert (pt->pt_range != NULL);	

	min_idx = pt->pt_min(ti->ti_diskoff);
	max_idx = pt->pt_max(ti->ti_diskoff);
	for (k = min_idx; k <= max_idx; k++) {
		new_ti.ti_diskoff = pt->pt_range(ti->ti_diskoff, k);
		scan_type(&new_ti);
	}
}

static void scan_type_pointsto_all(const struct type_info* ti)
{
	const struct fsl_rt_table_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_pointsto_c; i++) {
		scan_type_pointsto(ti, &tt->tt_pointsto[i]);
	}
}

/* dump all data for strong usertypes that are aggregate to the given type */
static void scan_type_strong_types(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);

	for (i = 0; i < tt->tt_field_c; i++) {
		struct fsl_rt_table_field*	field;
		uint64_t			bitoff;
		
		field = &tt->tt_field_thunkoff[i];
		bitoff = field->tf_fieldbitoff(ti->ti_diskoff);

		/* dump data */
		print_indent(ti->ti_depth);
		printf("%s::", field->tf_fieldname);

		printf("%s",
			(field->tf_typenum != ~0) ?
				tt_by_num(field->tf_typenum)->tt_name : 
				"ANONYMOUS");

		printf("@offset=0x%" PRIx64 " (%" PRIu64 ")\n", bitoff, bitoff);

		if (field->tf_typenum != ~0) {
			/* recurse */
			struct type_info	new_type;
			
			new_type.ti_typenum = field->tf_typenum;
			new_type.ti_diskoff = bitoff;
			new_type.ti_depth = ti->ti_depth + 1;

			scan_type(&new_type);
		}
	}
}

static void set_dyn_on_type(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	__setDyn(ti->ti_typenum, ti->ti_diskoff);

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_field_c; i++) {
		struct fsl_rt_table_field	*field;

		field = &tt->tt_field_thunkoff[i];
		if (field->tf_typenum == ~0)
			continue;
		__setDyn(
			field->tf_typenum, 
			field->tf_fieldbitoff(ti->ti_diskoff));
	}
}

static void scan_type(const struct type_info* ti)
{
	unsigned int i;

	set_dyn_on_type(ti);

	print_indent(ti->ti_depth-1);
	printf("scanning: %s (%d usertypes)\n", 
		tt_by_ti(ti)->tt_name,
		tt_by_ti(ti)->tt_field_c);
	scan_type_strong_types(ti);
	scan_type_pointsto_all(ti);
}

void tool_entry(void)
{
	struct type_info	origin_ti;
	unsigned int 		i;

	origin_ti.ti_typenum = fsl_rt_origin_typenum;
	origin_ti.ti_diskoff = 0;
	origin_ti.ti_depth = 1;

	for (i = 0; i < fsl_rt_table_entries; i++) {
		printf("%02d: %s\n", i, fsl_rt_table[i].tt_name);
	}

	scan_type(&origin_ti);
}


