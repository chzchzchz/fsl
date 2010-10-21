#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "debug.h"
#include "type_info.h"

static char hexmap[] = {"0123456789abcdef"};

static void typeinfo_print_path_helper(const struct type_info* ti)
{
	/* end of recursion? */
	if (ti == NULL)
		return;

	typeinfo_print_path_helper(ti->ti_prev);

	typeinfo_print(ti);
	printf("/");
}


static void print_field(
	const struct type_info		*ti,
	const struct fsl_rt_table_field	*field)
{
	uint64_t			num_elems;
	typesize_t			field_sz;
	typenum_t			field_typenum;
	diskoff_t			field_off;
	unsigned int			param_c;
	TI_INTO_CLO(ti);

	num_elems = field->tf_elemcount(clo);
	printf("%s", field->tf_fieldname);
	if (num_elems > 1) {
		printf("[%"PRIu64"]", num_elems);
	}

	field_off = field->tf_fieldbitoff(clo);

	field_typenum = field->tf_typenum;
	if (num_elems > 1 &&
	    field_typenum != TYPENUM_INVALID &&
	    !field->tf_constsize)
	{
		/* non-constant width.. */
		field_sz = __computeArrayBits(
			ti_typenum(ti), clo, field->tf_fieldnum, num_elems);
	} else {
		/* constant width */
		field_sz = field->tf_typesize(clo);
		field_sz *= num_elems;
	}

	NEW_CLO(last_field_clo, field_off, NULL);

	if (num_elems == 1 && field_sz <= 64) {
		uint64_t	v;
		v = __getLocal(&last_field_clo, field_off, field_sz);
		printf(" = %"PRIu64 " (0x%"PRIx64")", v, v);
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

static void typeinfo_print_type(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	uint64_t			len;
	TI_INTO_CLO			(ti);

	tt = tt_by_ti(ti);
	len = tt->tt_size(clo);

#ifdef PRINT_BITS
	printf("%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)",
		tt_by_ti(ti)->tt_name,
		ti_offset(ti),
		ti_offset(ti) + len,
		len);
#else
	printf("%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bytes)",
		tt_by_ti(ti)->tt_name,
		ti_offset(ti)/8,
		(ti_offset(ti) + len)/8,
		len/8);
#endif
}

static void typeinfo_print_field(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	struct fsl_rt_table_field	*field;
	uint64_t			len;
	TI_INTO_CLO			(ti->ti_prev);

	tt = tt_by_ti(ti->ti_prev);
	field = &tt->tt_fieldstrong_table[ti->ti_fieldidx];
	len = field->tf_typesize(clo);
#ifdef PRINT_BITS
	printf("%s.%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)",
		tt->tt_name,
		field->tf_fieldname,
		ti_offset(ti),
		ti_offset(ti) + len,
		len);
#else
	printf("%s.%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bytes)",
		tt->tt_name,
		field->tf_fieldname,
		ti_offset(ti)/8,
		(ti_offset(ti) + len)/8,
		len/8);
#endif
}

void typeinfo_print(const struct type_info* ti)
{
	assert (ti != NULL);

	if (ti->ti_prev == NULL || ti_typenum(ti) != TYPENUM_INVALID) {
		typeinfo_print_type(ti);
		return;
	}

	assert (ti->ti_prev != NULL);
	assert (ti->ti_pointsto == false);

	typeinfo_print_field(ti);
}

void typeinfo_print_fields(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	if (ti_typenum(ti) == TYPENUM_INVALID)
		return;

	tt = tt_by_ti(ti);

	if (tt->tt_fieldall_c > 0)
		printf("Fields:\n");

	for (i = 0; i < tt->tt_fieldall_c; i++) {
		struct fsl_rt_table_field	*field;
		bool				is_present;
		TI_INTO_CLO			(ti);

		field = &tt->tt_fieldall_thunkoff[i];
		if (field->tf_cond == NULL)
			is_present = true;
		else
			is_present = field->tf_cond(clo);

		if (is_present == false)
			continue;

		if (field->tf_typenum != TYPENUM_INVALID)
			printf("%02d. ", i);
		else
			printf("--- ");

		print_field(ti, field);
	}
}

void typeinfo_print_path(const struct type_info* ti)
{
	typeinfo_print_path_helper(ti->ti_prev);
	typeinfo_print(ti);
}

void typeinfo_print_pointsto(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	bool				none_seen;
	unsigned int			i;

	if (ti_typenum(ti) == TYPENUM_INVALID)
		return;

	tt = tt_by_ti(ti);

	none_seen = true;
	for (i = 0; i < tt->tt_pointsto_c; i++) {
		struct fsl_rt_table_pointsto	*pt;
		uint64_t			pt_min, pt_max;
		TI_INTO_CLO			(ti);

		pt = &tt->tt_pointsto[i];
		pt_min = pt->pt_min(clo);
		pt_max = pt->pt_max(clo);
		if (pt_min > pt_max) {
			/* failed some condition, don't display. */
			continue;
		}

		if (none_seen) {
			printf("Points-To:\n");
			none_seen = false;
		}

		if (pt->pt_name == NULL) {
			printf("%02d. (%s)\n",
				tt->tt_fieldall_c + i,
				tt_by_num(pt->pt_type_dst)->tt_name);
		} else {
			printf("%02d. %s (%s)\n",
				tt->tt_fieldall_c + i,
				pt->pt_name,
				tt_by_num(pt->pt_type_dst)->tt_name);
		}

	}
}

void typeinfo_print_virt(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;
	bool				none_seen;

	if (ti_typenum(ti) == TYPENUM_INVALID) return;
	tt = tt_by_ti(ti);

	none_seen = true;
	for (i = 0; i < tt->tt_virt_c; i++) {
		struct fsl_rt_table_virt	*vt;
		uint64_t			vt_min, vt_max;
		TI_INTO_CLO			(ti);

		vt = &tt->tt_virt[i];
		vt_min = vt->vt_min(clo);
		vt_max = vt->vt_max(clo);
		if (vt_min > vt_max)
			continue;

		if (none_seen) {
			printf("Virtuals: \n");
			none_seen = false;
		}

		if (vt->vt_name == NULL) {
			printf("%02d. (%s->%s)\n",
				tt->tt_fieldall_c + tt->tt_pointsto_c + i,
				tt_by_num(vt->vt_type_src)->tt_name,
				tt_by_num(vt->vt_type_virttype)->tt_name);
		} else {
			printf("%02d. %s (%s->%s)\n",
				tt->tt_fieldall_c + tt->tt_pointsto_c + i,
				vt->vt_name,
				tt_by_num(vt->vt_type_src)->tt_name,
				tt_by_num(vt->vt_type_virttype)->tt_name);
		}

	}
}

void typeinfo_dump_data(const struct type_info* ti)
{
	uint64_t			type_sz;
	unsigned int			i;
	TI_INTO_CLO			(ti);

	type_sz = tt_by_ti(ti)->tt_size(clo);

	for (i = 0; i < type_sz / 8; i++) {
		uint8_t	c;

		if ((i % 0x18) == 0) {
			if (i != 0) printf("\n");
			printf("%04x: ", i);
		}
		c = __getLocal(clo, ti_offset(ti) + (i*8), 8);
		printf("%c%c ", hexmap[(c >> 4) & 0xf], hexmap[c & 0xf]);

	}

	printf("\n");
	if (type_sz % 8)
		printf("OOPS-- size is not byte-aligned\n");
}
