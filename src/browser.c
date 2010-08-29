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
static bool handle_menu_choice(
	struct type_info* cur,
	int choice);

static int get_sel_elem(int min_v, int max_v);
static void select_field(struct type_info* cur, int field_num);
static void select_pointsto(struct type_info* cur, int pt_idx);

static void select_pointsto(struct type_info* cur, int pt_idx)
{
	struct fsl_rt_table_pointsto	*pt;
	struct type_info		*ti_next;
	struct fsl_rt_table_type	*tt_out;
	diskoff_t			pt_off;
	int				min_idx, max_idx;
	int				pt_elem_idx;

	pt = &(tt_by_ti(cur)->tt_pointsto[pt_idx]);

	/* get the range element idx */
	min_idx = pt->pt_min(cur->ti_diskoff, cur->ti_params);
	max_idx = pt->pt_max(cur->ti_diskoff, cur->ti_params);
	pt_elem_idx = get_sel_elem(min_idx, max_idx);
	if (pt_elem_idx == INT_MIN)
		return;

	tt_out = tt_by_num(pt->pt_type_dst);
	uint64_t	params[tt_out->tt_param_c];

	/* jump to it */
	pt_off = pt->pt_range(
		cur->ti_diskoff, cur->ti_params, pt_elem_idx, params);
	assert (0 == 1 && "DO SOMETHING WITH PARAMS.");
	ti_next = typeinfo_alloc_pointsto(
		pt->pt_type_dst, pt_off, pt_idx, pt_elem_idx, cur);
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

static void select_field(struct type_info* cur, int field_idx)
{
	struct fsl_rt_table_field	*field;
	struct type_info		*ti_next;
	uint64_t			num_elems;
	diskoff_t			next_diskoff;

	field = &(tt_by_ti(cur)->tt_fieldall_thunkoff[field_idx]);
	if (field->tf_typenum == ~0) {
		printf("Given field number does not point to type.\n");
		return;
	}

	num_elems = field->tf_elemcount(cur->ti_diskoff, cur->ti_params);
	next_diskoff = field->tf_fieldbitoff(
		cur->ti_diskoff, cur->ti_params);

	if (num_elems > 1) {
		int	sel_elem;

		sel_elem = get_sel_elem(0, num_elems-1);
		if (sel_elem < INT_MIN)
			return;

		if (field->tf_constsize == false) {
			typesize_t	array_off;

			assert (0 == 1 && "LOAD PARAMS PROPERLY");

			array_off = __computeArrayBits(
				field->tf_typenum,
				next_diskoff,
				NULL,
				sel_elem);

			next_diskoff += array_off;
		}  else {
			typesize_t	fsz;
			fsz = field->tf_typesize(
				cur->ti_diskoff, cur->ti_params);
			next_diskoff += sel_elem * fsz;
		}
	}

	ti_next = typeinfo_alloc(
		field->tf_typenum, next_diskoff, field_idx, cur);
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

	if (choice == -1) {
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

	printf("Welcome to fsl browser. Browse mode: \"%s\"\n", fsl_rt_fsname);

	origin_ti = typeinfo_alloc(fsl_rt_origin_typenum, 0, 0, NULL);
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		return;
	}

	menu(origin_ti);
	typeinfo_free(origin_ti);
	printf("Have a nice day.\n");
}


