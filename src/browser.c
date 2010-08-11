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
			unsigned int	ti_pointstoidx;
			unsigned int	ti_pointsto_elem; /* for arrays */
		};
	};

	struct type_info	*ti_prev;
};

#define INPUT_BUF_SZ	1024
static char input_buf[INPUT_BUF_SZ];

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
		len = field->tf_typesize(ti->ti_prev->ti_diskoff);
		printf("%s.%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)\n",
			tt->tt_name,
			field->tf_fieldname,
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
	if (tt->tt_pointsto_c > 0)
		printf("Points-To:\n");
	for (i = 0; i < tt->tt_pointsto_c; i++) {
		printf("%02d. ((%s))\n", 
			tt->tt_fieldall_c + i,
			tt_by_num(tt->tt_pointsto[i].pt_type_dst)->tt_name);
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

	if (num_elems > 1 || field_sz > 32) {
		printf("@%"PRIu64"--%"PRIu64"\n", 
			field_off, field_off + field_sz*num_elems);
	} else {
		printf(" = %"PRIu64"\n",
			__getLocal(field_off, field_sz));
	}
}


static void print_typeinfo_fields(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	if (ti->ti_typenum == ~0)
		return;

	tt = tt_by_ti(ti);

	if (tt->tt_fieldall_c > 0)
		printf("Fields:\n");

	for (i = 0; i < tt->tt_fieldall_c; i++) {
		struct fsl_rt_table_field	*field;
		bool				is_present;

		field = &tt->tt_fieldall_thunkoff[i]; 
		if (field->tf_cond == NULL)
			is_present = true;
		else
			is_present = field->tf_cond(ti->ti_diskoff);

		if (is_present == false)
			continue;

		if (field->tf_typenum != ~0)
			printf("%02d. ", i);
		else
			printf("--- ");

		print_field(field, ti->ti_diskoff);
	}
}


static char hexmap[] = {"0123456789abcdef"};
static void dump_typeinfo_data(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	uint64_t			type_sz;
	unsigned int			i;

	tt = tt_by_ti(ti);
	type_sz = tt->tt_size(ti->ti_diskoff);

	for (i = 0; i < type_sz / 8; i++) {
		uint8_t	c;

		if ((i % (80/3)) ==((80/3)-1))
			printf("\n");

		c = __getLocal(ti->ti_diskoff + (i*8), 8);
		printf("%c%c ", hexmap[(c >> 4) & 0xf], hexmap[c & 0xf]);

	}

	printf("\n");
	if (type_sz % 8)
		printf("OOPS-- size is not byte-aligned\n");
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


