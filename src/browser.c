#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "runtime.h" 

#define tt_by_num(x)	(&fsl_rt_table[x])
#define tt_by_ti(x)	tt_by_num((x)->ti_typenum)

struct type_info
{
	typenum_t		ti_typenum;
	diskoff_t		ti_diskoff;

	unsigned int		ti_fieldidx;
	struct type_info	*ti_prev;
};

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
	len = tt->tt_field_thunkoff[ti->ti_fieldidx].tt_typesize(
		ti->ti_prev->ti_diskoff);

	printf("%s.%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)\n",
		tt->tt_name,
		tt->tt_field_thunkoff[ti->ti_fieldidx].tt_fieldname,
		ti->ti_diskoff,
		ti->ti_diskoff+ len,
		len);
}

static void print_typeinfo_fields(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	if (ti->ti_typenum == ~0)
		return;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_num_fields; i++) {
		printf("%02d. %s\n", 
			i,
			tt->tt_field_thunkoff[i].tt_fieldname);
	}
}

static void typeinfo_free(struct type_info* ti)
{
	free(ti);
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
	ret->ti_fieldidx = ti_fieldidx;
	ret->ti_prev = ti_prev;

	set_dyn_on_type(ret);

	return ret;
}

static void menu(struct type_info* cur)
{
	do {
		int				choice;
		struct fsl_rt_table_field	*field;
		struct type_info		*ti_next;
		ssize_t				br;

		printf("Current: ");
		print_typeinfo(cur);

		printf("Fields:\n");
		print_typeinfo_fields(cur);

		br = fscanf(stdin, "%d", &choice);
		if (choice < 0 || br <= 0)
			return;

		if (choice >= tt_by_ti(cur)->tt_num_fields) {
			printf("Error: Bad field\n");
			continue;
		}

		field = &(tt_by_ti(cur)->tt_field_thunkoff[choice]);
		ti_next = typeinfo_alloc(
			field->tt_typenum,
			field->tt_fieldbitoff(cur->ti_diskoff),
			choice,
			cur);
			
		menu(ti_next);

		typeinfo_free(ti_next);
	} while(1);
}


void tool_entry(void)
{
	struct type_info	*origin_ti;
	unsigned int 		i;

	origin_ti = typeinfo_alloc(fsl_rt_origin_typenum, 0, 0, NULL);

	printf("fsl browser. Browsing your filesystem.\n");

	menu(origin_ti);
}


