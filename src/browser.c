/* browse a filesystem like a pro */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <stdbool.h>
#include "runtime.h" 

#define tt_by_num(x)	(&fsl_rt_table[x])
#define tt_by_ti(x)	tt_by_num((x)->ti_typenum)

struct type_info
{
	typenum_t		ti_typenum;
	diskoff_t		ti_diskoff;

	bool			ti_pointsto;
	union {
		unsigned int		ti_fieldidx;
		struct {
			unsigned int		ti_pointstoidx;
			unsigned int		ti_pointsto_elem; /* for arrays */
		};
	};

	struct type_info	*ti_prev;
};

static void set_dyn_on_type(const struct type_info* ti);
static void print_typeinfo(const struct type_info* ti);
static void print_typeinfo_fields(const struct type_info* ti);
static void typeinfo_free(struct type_info* ti);
static struct type_info* typeinfo_alloc(
	typenum_t		ti_typenum,
	diskoff_t		ti_diskoff,
	unsigned int		ti_fieldidx,
	struct type_info*	ti_prev);
static struct type_info* typeinfo_alloc_pointsto(
	typenum_t		ti_typenum,
	diskoff_t		ti_diskoff,
	unsigned int		ti_pointsto_idx,
	unsigned int		ti_pointsto_elem,
	struct type_info*	ti_prev);

static void print_typeinfo_pointsto(const struct type_info* cur);
static void menu(struct type_info* cur);
static void handle_menu_choice(
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
	for (i = 0; i < tt->tt_num_fields; i++) {
		struct fsl_rt_table_field	*field;

		field = &tt->tt_field_thunkoff[i];
		if (field->tt_typenum == ~0)
			continue;
		__setDyn(
			field->tt_typenum, 
			field->tt_fieldbitoff(ti->ti_diskoff));
	}
}

static void print_typeinfo(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	uint64_t			len;

	assert (ti != NULL);

	tt = tt_by_ti(ti);
	if (ti->ti_prev == NULL || ti->ti_typenum != ~0) {
		len = tt->tt_size(ti->ti_diskoff);

		printf("%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)\n", 
			tt_by_ti(ti)->tt_name, 
			ti->ti_diskoff,
			ti->ti_diskoff + len,
			len);
		return;
	}

	assert (ti->ti_prev != NULL);

	tt = tt_by_ti(ti->ti_prev);
	if (ti->ti_pointsto) {
		/* XXX STUB */
		assert (0 == 1);
	} else {
		struct fsl_rt_table_field	*field;

		field = &tt->tt_field_thunkoff[ti->ti_fieldidx];
		len = field->tt_typesize(ti->ti_prev->ti_diskoff);
		printf("%s.%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)\n",
			tt->tt_name,
			field->tt_fieldname,
			ti->ti_diskoff,
			ti->ti_diskoff + len,
			len);
	}
}

static void print_typeinfo_pointsto(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	if (ti->ti_typenum == ~0)
		return;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_num_pointsto; i++) {
		printf("%02d. ((%s))\n", 
			tt->tt_num_fields + i,
			tt_by_num(tt->tt_pointsto[i].pt_type_dst)->tt_name);
	}
}

static void print_typeinfo_fields(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	if (ti->ti_typenum == ~0)
		return;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_num_fields; i++) {
		struct fsl_rt_table_field	*field;
		uint64_t			num_elems;
		typesize_t			field_sz;
		diskoff_t			field_off;

		field = &tt->tt_field_thunkoff[i];
		num_elems = field->tt_elemcount(ti->ti_diskoff);

		if (num_elems == 1) {
			printf("%02d. %s", 
				i,
				tt->tt_field_thunkoff[i].tt_fieldname);
		} else {
			printf("%02d. %s[%"PRIu64"]", 
				i,
				tt->tt_field_thunkoff[i].tt_fieldname,
				num_elems);
		}

		field_off = field->tt_fieldbitoff(ti->ti_diskoff);
		field_sz = field->tt_typesize(ti->ti_diskoff);

		printf("@%"PRIu64"--%"PRIu64"\n", 
			field_off, field_off + field_sz*num_elems);

	}
}

static void typeinfo_free(struct type_info* ti)
{
	free(ti);
}

static struct type_info* typeinfo_alloc_pointsto(
	typenum_t		ti_typenum,
	diskoff_t		ti_diskoff,
	unsigned int		ti_pointsto_idx,
	unsigned int		ti_pointsto_elem,
	struct type_info*	ti_prev)
{
	struct type_info*	ret;

	ret = malloc(sizeof(struct type_info));
	ret->ti_typenum = ti_typenum;
	ret->ti_diskoff = ti_diskoff;

	ret->ti_pointsto = true;
	ret->ti_pointstoidx = ti_pointsto_idx;
	ret->ti_pointsto_elem = ti_pointsto_elem;

	ret->ti_prev = ti_prev;

	set_dyn_on_type(ret);

	return ret;
}


static struct type_info* typeinfo_alloc(
	typenum_t		ti_typenum,
	diskoff_t		ti_diskoff,
	unsigned int		ti_fieldidx,
	struct type_info*	ti_prev)
{
	struct type_info*	ret;

	ret = malloc(sizeof(struct type_info));
	ret->ti_typenum = ti_typenum;
	ret->ti_diskoff = ti_diskoff;
	
	ret->ti_pointsto = false;
	ret->ti_fieldidx = ti_fieldidx;

	ret->ti_prev = ti_prev;

	set_dyn_on_type(ret);

	return ret;

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

	field = &(tt_by_ti(cur)->tt_field_thunkoff[field_idx]);
	ti_next = typeinfo_alloc(
		field->tt_typenum,
		field->tt_fieldbitoff(cur->ti_diskoff),
		field_idx,
		cur);
		
	menu(ti_next);

	typeinfo_free(ti_next);
}

static void handle_menu_choice(
	struct type_info* cur,
	int choice)
{
	struct fsl_rt_table_type	*tt;

	tt = tt_by_ti(cur);
	if (choice < tt->tt_num_fields) {
		select_field(cur, choice);
		return;
	}

	choice -= tt->tt_num_fields;
	if (choice < tt->tt_num_pointsto) {
		select_pointsto(cur, choice);
		return;
	}


	printf("Error: Bad field value.\n");
}


static void menu(struct type_info* cur)
{
	do {
		int				choice;
		ssize_t				br;

		printf("Current: ");
		print_typeinfo(cur);

		printf("Fields:\n");
		print_typeinfo_fields(cur);

		printf("Points-To:\n");
		print_typeinfo_pointsto(cur);

		br = fscanf(stdin, "%d", &choice);
		if (br <= 0 || choice < 0)
			return;

		handle_menu_choice(cur, choice);
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


