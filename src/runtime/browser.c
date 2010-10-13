/* browse a filesystem like a pro */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>
#include <stdbool.h>

#include "runtime.h"
#include "type_info.h"

static void menu(struct type_info* cur);
static bool handle_menu_choice(struct type_info* cur, int choice);

static int get_sel_elem(int min_v, int max_v);
static void select_field(struct type_info* cur, int field_num);
static void select_pointsto(struct type_info* cur, int pt_idx);

static void select_pointsto(struct type_info* cur, int pt_idx)
{
	struct fsl_rt_table_pointsto	*pt;
	struct type_info		*ti_next;
	struct type_desc		next_td;
	int				pt_elem_idx;
	unsigned int			min_idx, max_idx;
	TI_INTO_CLO			(cur);

	pt = &(tt_by_ti(cur)->tt_pointsto[pt_idx]);

	/* get the range element idx */
	min_idx = pt->pt_min(clo);
	max_idx = pt->pt_max(clo);
	if (min_idx != max_idx) {
		pt_elem_idx = get_sel_elem(min_idx, max_idx);
		if (pt_elem_idx == INT_MIN)
			return;
	} else {
		/* only one element -- follow that */
		pt_elem_idx = min_idx;
	}

	uint64_t	params[tt_by_num(pt->pt_type_dst)->tt_param_c];

	/* jump to it */
	td_init(&next_td,
		pt->pt_type_dst,
		pt->pt_range(clo, pt_elem_idx, params),
		params);

	ti_next = typeinfo_alloc_pointsto(&next_td, pt_idx, pt_elem_idx, cur);
	if (ti_next == NULL)
		return;

	menu(ti_next);

	typeinfo_free(ti_next);
}

static int get_sel_elem(int min_v, int max_v)
{
	int	sel_elem;

	assert (min_v > INT_MIN);
	assert (max_v < INT_MAX);

	if (min_v > max_v) return INT_MAX;
	if (min_v == max_v) return min_v;

	sel_elem = INT_MIN;
	while (sel_elem < min_v || sel_elem > max_v) {
		ssize_t	br;

		printf("Which element of [%"PRIi32",%"PRIi32"]?\n>> ", min_v, max_v);
		br = fscanf(stdin, "%d", &sel_elem);
		if (br < 0)
			return INT_MIN;
	}

	return sel_elem;
}

static uint64_t select_field_array(
	struct type_info* cur,
	struct fsl_rt_table_field* field,
	unsigned int num_elems,
	uint64_t next_diskoff,
	uint64_t* sel_idx)
{
	int		sel_elem;
	TI_INTO_CLO	(cur);

	assert (sel_idx != NULL);

	sel_elem = get_sel_elem(0, num_elems-1);
	if (sel_elem < INT_MIN)
		return ~0;
	*sel_idx = sel_elem;

	if (field->tf_constsize == false) {
		typesize_t	array_off;

		array_off = __computeArrayBits(
			ti_typenum(cur), clo,
			field->tf_fieldnum,
			sel_elem);

		next_diskoff += array_off;
	} else {
		typesize_t	fsz;
		fsz = field->tf_typesize(clo);
		next_diskoff += sel_elem * fsz;
	}

	return next_diskoff;
}

static void select_field(struct type_info* cur, int field_idx)
{
	struct fsl_rt_table_field	*field;
	struct type_info		*ti_next;
	uint64_t			num_elems;
	uint64_t			params[tt_by_ti(cur)->tt_param_c];
	struct type_desc		next_td;
	diskoff_t			next_off;
	TI_INTO_CLO			(cur);

	field = &(tt_by_ti(cur)->tt_fieldall_thunkoff[field_idx]);
	if (field->tf_typenum == TYPENUM_INVALID) {
		printf("Given field number does not point to type.\n");
		return;
	}

	next_off = field->tf_fieldbitoff(clo);
	num_elems = field->tf_elemcount(clo);

	if (num_elems > 1) {
		uint64_t	sel_idx;
		next_off = select_field_array(
			cur, field, num_elems, next_off, &sel_idx);
		if (next_off != ~0)
			field->tf_params(clo, sel_idx, params);
	} else {
		field->tf_params(clo, 0, params);
	}

	if (next_off == ~0)
		return;

	td_init(&next_td, field->tf_typenum, next_off, params);

	ti_next = typeinfo_alloc(&next_td, field_idx, cur);
	if (ti_next == NULL)
		return;

	menu(ti_next);

	typeinfo_free(ti_next);
}

static void select_virt(struct type_info* cur, int vt_idx)
{
	struct type_info*		ti_next;
	struct fsl_rt_table_virt*	vt;

	vt = &tt_by_ti(cur)->tt_virt[vt_idx];
	ti_next = typeinfo_alloc_virt(vt, cur);
	if (ti_next == NULL)
		return;

	menu(ti_next);

	typeinfo_free(ti_next);
}

#define MCMD_DUMP	-1

/**
 * false -> go back a level from when entered
 * true -> stay on same level as when entered
 */
static bool handle_menu_choice(
	struct type_info* cur,
	int choice)
{
	struct fsl_rt_table_type	*tt;

	if (choice == MCMD_DUMP) {
		/* dump all of current type */
		typeinfo_dump_data(cur);
		return true;
	}

	if (choice < 0)
		return false;

	tt = tt_by_ti(cur);
	if (choice < tt->tt_fieldall_c) {
		select_field(cur, choice);
		return true;
	}

	choice -= tt->tt_fieldall_c;
	if (choice < tt->tt_pointsto_c) {
		select_pointsto(cur, choice);
		return true;
	}

	choice -= tt->tt_pointsto_c;
	if (choice < tt->tt_virt_c) {
		select_virt(cur, choice);
		return true;
	}

	printf("Error: Bad field value.\n");
	return true;
}

static void menu(struct type_info* cur)
{
	do {
		int				choice;
		ssize_t				br;

		printf("Current: ");
		typeinfo_print(cur);
		printf("\n");
		typeinfo_print_path(cur);
		printf("\n");

		typeinfo_print_fields(cur);
		typeinfo_print_pointsto(cur);
		typeinfo_print_virt(cur);

		printf(">> ");
		br = fscanf(stdin, "%d", &choice);
		if ((br <= 0) || !handle_menu_choice(cur, choice))
			return;
	} while(1);
}

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;
	struct type_desc	init_td = td_origin();

	printf("Welcome to fsl browser. Browse mode: \"%s\"\n", fsl_rt_fsname);

	origin_ti = typeinfo_alloc(&init_td, 0, NULL);
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		return -1;
	}

	menu(origin_ti);
	typeinfo_free(origin_ti);
	printf("Have a nice day.\n");

	return 0;
}
