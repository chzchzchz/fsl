#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "type_info.h"

static char hexmap[] = {"0123456789abcdef"};

void typeinfo_print(const struct type_info* ti)
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

void typeinfo_print_pointsto(const struct type_info* ti)
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

void typeinfo_print_fields(const struct type_info* ti)
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

struct type_info* typeinfo_alloc_pointsto(
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

void typeinfo_free(struct type_info* ti)
{
	free(ti);
}



void typeinfo_dump_data(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	uint64_t			type_sz;
	unsigned int			i;

	tt = tt_by_ti(ti);
	type_sz = tt->tt_size(ti->ti_diskoff);

	for (i = 0; i < type_sz / 8; i++) {
		uint8_t	c;

		if ((i % 0x18) == 0) {
			if (i != 0) printf("\n");
			printf("%04x: ", i);
		}
		c = __getLocal(ti->ti_diskoff + (i*8), 8);
		printf("%c%c ", hexmap[(c >> 4) & 0xf], hexmap[c & 0xf]);

	}

	printf("\n");
	if (type_sz % 8)
		printf("OOPS-- size is not byte-aligned\n");
}


struct type_info* typeinfo_alloc(
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


