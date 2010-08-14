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

static void set_dyn_on_type(const struct type_info* ti);
static void menu(struct type_info* cur);
static bool handle_menu_choice(
	struct type_info* cur,
	int choice);

static void select_field(struct type_info* cur, int field_num);
static void select_pointsto(struct type_info* cur, int pt_idx);


static void set_dyn_on_type(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	if (ti->ti_typenum == ~0)
		return;

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

static void print_field(
	const struct fsl_rt_table_field* field,
	diskoff_t ti_diskoff)
{
	uint64_t			num_elems;
	typesize_t			field_sz;
	diskoff_t			field_off;

	num_elems = field->tf_elemcount(ti_diskoff);
	printf("%s", field->tf_fieldname);
	if (num_elems > 1) {
		printf("[%"PRIu64"]", num_elems);
	}

	field_off = field->tf_fieldbitoff(ti_diskoff);
	field_sz = field->tf_typesize(ti_diskoff);

	if (num_elems == 1 && field_sz <= 64) {
		printf(" = %"PRIu64, __getLocal(field_off, field_sz));
	} else
		printf("@%"PRIu64"--%"PRIu64, 
			field_off, field_off + field_sz*num_elems);

	printf("\n");
}

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
		typesize_t	fsz;
		int		sel_elem;
		ssize_t		br;

		fsz = field->tf_typesize(cur->ti_diskoff);

		sel_elem = num_elems + 1;
		while (sel_elem >= num_elems) {
			printf("Which element? (of %"PRIu64")\n>> ", num_elems);
			br = fscanf(stdin, "%d", &sel_elem);
			if (br < 0 || sel_elem < 0)
				return;
		}

		next_diskoff += sel_elem * fsz;
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
		dump_typeinfo_data(cur);
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
		print_typeinfo(cur);

		print_typeinfo_fields(cur);

		print_typeinfo_pointsto(cur);

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


