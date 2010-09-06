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

#define INPUT_BUF_SZ	1024
static char input_buf[INPUT_BUF_SZ];

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

	pt = &(tt_by_ti(cur)->tt_pointsto[pt_idx]);

	/* get the range element idx */
	min_idx = pt->pt_min(ti_to_thunk(cur));
	max_idx = pt->pt_max(ti_to_thunk(cur));
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
	next_td.td_diskoff = pt->pt_range(
		ti_to_thunk(cur), pt_elem_idx, params);
	next_td.td_typenum = pt->pt_type_dst;
	next_td.td_params = params;

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
	parambuf_t next_params)
{
	int	sel_elem;

	sel_elem = get_sel_elem(0, num_elems-1);
	if (sel_elem < INT_MIN)
		return;

	if (field->tf_constsize == false) {
		typesize_t	array_off;

		array_off = __computeArrayBits(
			field->tf_typenum,
			next_diskoff,
			next_params,
			sel_elem);

		next_diskoff += array_off;
	} else {
		typesize_t	fsz;
		fsz = field->tf_typesize(ti_to_thunk(cur));
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

	field = &(tt_by_ti(cur)->tt_fieldall_thunkoff[field_idx]);
	if (field->tf_typenum == ~0) {
		printf("Given field number does not point to type.\n");
		return;
	}

	next_td.td_typenum = field->tf_typenum;
	next_td.td_diskoff = field->tf_fieldbitoff(ti_to_thunk(cur));
	field->tf_params(ti_to_thunk(cur), params);
	next_td.td_params = params;

	num_elems = field->tf_elemcount(ti_to_thunk(cur));
	if (num_elems > 1) {
		next_td.td_diskoff = select_field_array(
			cur, field, num_elems, next_td.td_diskoff, params);
	}

	ti_next = typeinfo_alloc(&next_td, field_idx, cur);
	if (ti_next == NULL)
		return;

	menu(ti_next);

	typeinfo_free(ti_next);
}

static void select_virt(struct type_info* cur, int pt_idx)
{
	assert (0 == 1 && "SELECT VIRT STUB");
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

		printf(">> ");
		br = fscanf(stdin, "%d", &choice);
		if ((br <= 0) || !handle_menu_choice(cur, choice))
			return;
	} while(1);
}

void tool_entry(void)
{
	struct type_info	*origin_ti;
	struct type_desc	init_td = {fsl_rt_origin_typenum, 0, NULL};

	printf("Welcome to fsl browser. Browse mode: \"%s\"\n", fsl_rt_fsname);

	origin_ti = typeinfo_alloc(&init_td, 0, NULL);
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		return;
	}

	menu(origin_ti);
	typeinfo_free(origin_ti);
	printf("Have a nice day.\n");
}
