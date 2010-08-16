/* browse a filesystem like a pro */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
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

static void select_field(struct type_info* cur, int field_num);
static void select_pointsto(struct type_info* cur, int pt_idx);

static void select_pointsto(struct type_info* cur, int pt_idx)
{
	struct fsl_rt_table_pointsto	*pt;
	struct type_info		*ti_next;
	diskoff_t			pt_off;

	pt = &(tt_by_ti(cur)->tt_pointsto[pt_idx]);
	if (pt->pt_single == NULL) {
		printf("XXX: only single points-to supported\n");
		return;
	}

	pt_off = pt->pt_single(cur->ti_diskoff);

	ti_next = typeinfo_alloc_pointsto(
		pt->pt_type_dst, pt_off, pt_idx, 0, cur);
		
	menu(ti_next);

	typeinfo_free(ti_next);
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

	num_elems = field->tf_elemcount(cur->ti_diskoff);
	next_diskoff = field->tf_fieldbitoff(cur->ti_diskoff);

	if (num_elems > 1) {
		int		sel_elem;
		ssize_t		br;

		sel_elem = num_elems + 1;
		while (sel_elem >= num_elems) {
			printf("Which element? (of %"PRIu64")\n>> ", num_elems);
			br = fscanf(stdin, "%d", &sel_elem);
			if (br < 0 || sel_elem < 0)
				return;
		}

		if (field->tf_constsize == false) {
			typesize_t	array_off;

			array_off = __computeArrayBits(
				field->tf_typenum,
				next_diskoff,
				sel_elem);

			next_diskoff += array_off;
		}  else {
			typesize_t	fsz;
			fsz = field->tf_typesize(cur->ti_diskoff);
			next_diskoff += sel_elem * fsz;
		}
	}

	ti_next = typeinfo_alloc(
		field->tf_typenum, next_diskoff, field_idx, cur);
		
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

	printf("Welcome to fsl browser. Browsing \"%s\"\n", fsl_rt_fsname);
	origin_ti = typeinfo_alloc(fsl_rt_origin_typenum, 0, 0, NULL);
	menu(origin_ti);
	typeinfo_free(origin_ti);
	printf("Have a nice day.\n");
}


