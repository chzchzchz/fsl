#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "type_info.h"

static char hexmap[] = {"0123456789abcdef"};


static bool verify_asserts(const struct type_info *ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	assert (ti != NULL);

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_assert_c; i++) {
		const struct fsl_rt_table_assert	*as;
		as = &tt->tt_assert[i];
		if (as->as_assertf(ti->ti_diskoff) == false) {
			printf("!!! !!!Assert #%d failed on type %s!!! !!!\n", 
				i, tt->tt_name);
			return false;
		}
	}

	return true;
}

static void print_field(
	const struct type_info		*ti,
	const struct fsl_rt_table_field	*field)
{
	uint64_t			num_elems;
	typesize_t			field_sz;
	typenum_t			field_typenum;
	diskoff_t			field_off;

	num_elems = field->tf_elemcount(ti->ti_diskoff);
	printf("%s", field->tf_fieldname);
	if (num_elems > 1) {
		printf("[%"PRIu64"]", num_elems);
	}

	field_off = field->tf_fieldbitoff(ti->ti_diskoff);

	field_typenum = field->tf_typenum;
	if (num_elems > 1 && field_typenum != ~0 && !field->tf_constsize) {
		/* non-constant width.. */
		field_sz = __computeArrayBits(
			field_typenum,
			field_off,
			num_elems);
	} else {
		/* constant width */
		field_sz = field->tf_typesize(ti->ti_diskoff)*num_elems;
	}

	if (num_elems == 1 && field_sz <= 64) {
		printf(" = %"PRIu64 " (0x%"PRIx64")", 
			__getLocal(field_off, field_sz),
			__getLocal(field_off, field_sz));
	} else {
#ifdef PRINT_BITS
		printf("@%"PRIu64"--%"PRIu64, 
			field_off, field_off + field_sz);
#else
		printf("@%"PRIu64"--%"PRIu64, 
			field_off/8, (field_off + field_sz)/8);
#endif
	}

	printf("\n");
}

void typeinfo_print(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	uint64_t			len;

	assert (ti != NULL);

	tt = tt_by_ti(ti);
	if (ti->ti_prev == NULL || ti->ti_typenum != ~0) {
		len = tt->tt_size(ti->ti_diskoff);

#ifdef PRINT_BITS
		printf("%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)", 
			tt_by_ti(ti)->tt_name, 
			ti->ti_diskoff,
			ti->ti_diskoff + len,
			len);
#else
		printf("%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bytes)", 
			tt_by_ti(ti)->tt_name, 
			ti->ti_diskoff/8,
			(ti->ti_diskoff + len)/8,
			len/8);
#endif
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
#ifdef PRINT_BITS
		printf("%s.%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)",
			tt->tt_name,
			field->tf_fieldname,
			ti->ti_diskoff,
			ti->ti_diskoff + len,
			len);
#else
		printf("%s.%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bytse)",
			tt->tt_name,
			field->tf_fieldname,
			ti->ti_diskoff/8,
			(ti->ti_diskoff + len)/8,
			len/8);
#endif
	}
}

void typeinfo_print_pointsto(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	bool				none_seen;
	unsigned int			i;

	if (ti->ti_typenum == ~0)
		return;

	tt = tt_by_ti(ti);

	none_seen = true;
	for (i = 0; i < tt->tt_pointsto_c; i++) {
		struct fsl_rt_table_pointsto	*pt;

		pt = &tt->tt_pointsto[i];
		if (pt->pt_single == NULL) {
			uint64_t	pt_min, pt_max;

			pt_min = pt->pt_min(ti->ti_diskoff);
			pt_max = pt->pt_max(ti->ti_diskoff);
			if (pt_min > pt_max) {
				/* failed some condition */
				continue;
			}
		}

		if (none_seen) {
			printf("Points-To:\n");
			none_seen = false;
		}

		printf("%02d. (%s)\n", 
			tt->tt_fieldall_c + i,
			tt_by_num(pt->pt_type_dst)->tt_name);

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

		print_field(ti, field);
	}
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

struct type_info* typeinfo_alloc_pointsto(
	typenum_t		ti_typenum,
	diskoff_t		ti_diskoff,
	unsigned int		ti_pointsto_idx,
	unsigned int		ti_pointsto_elem,
	const struct type_info*	ti_prev)
{
	struct type_info*	ret;

	ret = malloc(sizeof(struct type_info));
	ret->ti_typenum = ti_typenum;
	ret->ti_diskoff = ti_diskoff;

	ret->ti_pointsto = true;
	ret->ti_pointstoidx = ti_pointsto_idx;
	ret->ti_pointsto_elem = ti_pointsto_elem;

	ret->ti_prev = ti_prev;
	if (ti_prev != NULL)
		ret->ti_depth = ti_prev->ti_depth + 1;

	typeinfo_set_dyn(ret);
	if (verify_asserts(ret) == false) {
		typeinfo_free(ret);
		return NULL;
	}

	return ret;
}

struct type_info* typeinfo_alloc(
	typenum_t		ti_typenum,
	diskoff_t		ti_diskoff,
	unsigned int		ti_fieldidx,
	const struct type_info*	ti_prev)
{
	struct type_info*	ret;

	ret = malloc(sizeof(struct type_info));
	ret->ti_typenum = ti_typenum;
	ret->ti_diskoff = ti_diskoff;
	
	ret->ti_pointsto = false;
	ret->ti_fieldidx = ti_fieldidx;

	ret->ti_prev = ti_prev;
	if (ti_prev != NULL) {
		ret->ti_depth = ti_prev->ti_depth + 1;
	}
	
	typeinfo_set_dyn(ret);
	if (verify_asserts(ret) == false) {
		typeinfo_free(ret);
		return NULL;
	}

	return ret;

}

void typeinfo_set_dyn(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

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

static void typeinfo_print_path_helper(const struct type_info* ti)
{
	/* end of recursion? */
	if (ti == NULL)
		return;

	typeinfo_print_path_helper(ti->ti_prev);

	typeinfo_print(ti);
	printf("/");
}


void typeinfo_print_path(const struct type_info* ti)
{
	typeinfo_print_path_helper(ti->ti_prev);
	typeinfo_print(ti);
}
