/* browse a filesystem like a pro */
//#define DEBUG_TOOL
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>
#include <stdbool.h>

#include "debug.h"
#include "runtime.h"
#include "type_info.h"

static void menu(struct type_info* cur);
static bool handle_menu_choice(struct type_info* cur, int choice);

static int get_sel_elem(int min_v, int max_v);
static void select_field(struct type_info* cur, int field_num);
static void select_pointsto(struct type_info* cur, int pt_idx);

static void select_pointsto(struct type_info* cur, int pt_idx)
{
	const struct fsl_rtt_pointsto	*pt;
	struct type_info			*ti_next;
	int					pt_elem_idx;
	unsigned int				min_idx, max_idx;

	fsl_env->fctx_failed_assert = NULL;
	pt = &(tt_by_ti(cur)->tt_pointsto[pt_idx]);

	/* get the range element idx */
	min_idx = pt->pt_iter.it_min(&ti_clo(cur));
	max_idx = pt->pt_iter.it_max(&ti_clo(cur));
	if (min_idx != max_idx) {
		pt_elem_idx = get_sel_elem(min_idx, max_idx);
		if (pt_elem_idx == INT_MIN)
			return;
	} else {
		/* only one element -- follow that */
		pt_elem_idx = min_idx;
	}

	ti_next = typeinfo_follow_pointsto(cur, pt, pt_elem_idx);
	if (ti_next == NULL) {
		printf("Could not select points-to.\n");
		if (fsl_env->fctx_failed_assert != NULL)
			printf("Reason: %s\n", fsl_env->fctx_failed_assert);
		return;
	}

	menu(ti_next);

	typeinfo_free(ti_next);
}

#define get_sel_elem_unbounded()	get_sel_elem(0, INT_MAX-1)

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

		printf("Which element of ");
		if (max_v == INT_MAX-1)
			printf("[%"PRIi32",...]?\n>> ", min_v);
		else
			printf("[%"PRIi32",%"PRIi32"]?\n>> ", min_v, max_v);

		br = fscanf(stdin, "%d", &sel_elem);
		if (br < 0)
			return INT_MIN;
	}

	return sel_elem;
}

static uint64_t select_field_array(
	struct type_info* cur,
	const struct fsl_rtt_field* field,
	unsigned int num_elems,
	uint64_t next_diskoff,
	uint64_t* sel_idx)
{
	int		sel_elem;
	TI_INTO_CLO	(cur);

	assert (sel_idx != NULL);

	sel_elem = get_sel_elem(0, num_elems-1);
	if (sel_elem == INT_MIN)
		return ~0;
	*sel_idx = sel_elem;

	if ((field->tf_flags & (FIELD_FL_CONSTSIZE | FIELD_FL_FIXED)) == 0) {
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
	const struct fsl_rtt_field	*field;
	struct type_info		*ti_next;
	uint64_t			num_elems;
	diskoff_t			next_off;
	uint64_t			sel_idx;

	DEBUG_TOOL_ENTER();

	field = &(tt_by_ti(cur)->tt_fieldall_thunkoff[field_idx]);
	if (field->tf_typenum == TYPENUM_INVALID) {
		printf("Given field number does not point to type.\n");
		goto done;
	}

	next_off = field->tf_fieldbitoff(&ti_clo(cur));
	num_elems = field->tf_elemcount(&ti_clo(cur));

	sel_idx = 0;
	if (num_elems > 1) {
		next_off = select_field_array(
			cur, field, num_elems, next_off, &sel_idx);
	}

	if (next_off == OFFSET_INVALID) {
		printf("No dice. Bad offset.\n");
		goto done;
	}

	fsl_err_reset();
	ti_next = typeinfo_follow_field_off_idx(cur, field, next_off, sel_idx);
	if (ti_next == NULL) {
		printf("No dice. Couldn't follow field idx=%"PRIu64".\n", sel_idx);
		if (fsl_err_get()) printf("Reason: %s\n", fsl_err_get());
		goto done;
	}

	menu(ti_next);

	typeinfo_free(ti_next);

done:
	DEBUG_TOOL_LEAVE();
}

static void select_virt(struct type_info* cur, int vt_idx)
{
	const struct fsl_rtt_type*	tt;
	const struct fsl_rtt_virt*	vt;
	struct type_info*		ti_next;
	int				sel_elem;
	int				err;

	DEBUG_TOOL_ENTER();

	tt = tt_by_ti(cur);
	assert (vt_idx < tt->tt_virt_c && "Virt index out of range");

	sel_elem = get_sel_elem_unbounded();
	if (sel_elem == INT_MIN)
		goto done;

	vt = &tt->tt_virt[vt_idx];
	err = 0;
	ti_next = typeinfo_follow_virt(cur, vt, sel_elem, &err);
	DEBUG_TOOL_WRITE("virt_alloc %s[%d]: %p-- err=%d\n",
		vt->vt_name, sel_elem, ti_next, err);
	if (ti_next == NULL) {
		printf("No dice. Couldn't follow virt type.\n");
		if (fsl_err_get()) printf("Reason: %s\n", fsl_err_get());
		goto done;
	}

	menu(ti_next);

	typeinfo_free(ti_next);
done:
	DEBUG_TOOL_LEAVE();
}

#define MCMD_DUMP	-1

/**
 * false -> go back a level from when entered
 * true -> stay on same level as when entered
 */
static bool handle_menu_choice(struct type_info* cur, int choice)
{
const struct fsl_rtt_type	*tt;

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
	DEBUG_TOOL_ENTER();
	do {
		int				choice;
		ssize_t				br;

		printf("Current: ");
		typeinfo_print(cur);
		printf("\n");
		typeinfo_print_path(cur);
		printf("\n");

		typeinfo_print_fields(cur);
		typeinfo_print_pointstos(cur);
		typeinfo_print_virts(cur);

		printf(">> ");
		br = fscanf(stdin, "%d", &choice);
		if ((br <= 0) || !handle_menu_choice(cur, choice))
			return;
	} while(1);
	DEBUG_TOOL_LEAVE();
}

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;

	printf("Welcome to fsl browser. Browse mode: \"%s\"\n", fsl_rt_fsname);

	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		printf("Failed assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	menu(origin_ti);
	typeinfo_free(origin_ti);
	printf("Have a nice day.\n");

	return 0;
}
