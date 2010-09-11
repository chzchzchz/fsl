#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

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
		if (as->as_assertf(ti_to_thunk(ti)) == false) {
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
	unsigned int			param_c;

	num_elems = field->tf_elemcount(ti_to_thunk(ti));
	printf("%s", field->tf_fieldname);
	if (num_elems > 1) {
		printf("[%"PRIu64"]", num_elems);
	}

	field_off = field->tf_fieldbitoff(ti_to_thunk(ti));

	field_typenum = field->tf_typenum;
	if (num_elems > 1 && field_typenum != TYPENUM_INVALID && !field->tf_constsize) {
		unsigned int param_c = tt_by_num(field->tf_typenum)->tt_param_c;
		uint64_t field_params[param_c];

		field->tf_params(ti_to_thunk(ti), field_params);

		/* non-constant width.. */
		field_sz = __computeArrayBits(
			field_typenum,
			field_off,
			field_params,
			num_elems);
	} else {
		/* constant width */
		field_sz = field->tf_typesize(ti_to_thunk(ti));
		field_sz *= num_elems;
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

static void typeinfo_print_type(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	uint64_t			len;

	tt = tt_by_ti(ti);
	if (ti->ti_is_virt) {
		printf("It's a virt!\n");
	}
	/* should not be resolving with xlate!*/
	printf("type=%s/off=%"PRIu64"\n",
		tt->tt_name,  ti->ti_td.td_diskoff);
	len = tt->tt_size(ti_to_thunk(ti));
	printf("GOT SIZE!\n");

#ifdef PRINT_BITS
	printf("%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)",
		tt_by_ti(ti)->tt_name,
		ti->ti_td.td_diskoff,
		ti->ti_td.td_diskoff + len,
		len);
#else
	printf("%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bytes)",
		tt_by_ti(ti)->tt_name,
		ti->ti_td.td_diskoff/8,
		(ti->ti_td.td_diskoff + len)/8,
		len/8);
#endif
}

static void typeinfo_print_field(const struct type_info* ti)
{
	struct fsl_rt_table_type	*tt;
	struct fsl_rt_table_field	*field;
	uint64_t			len;

	tt = tt_by_ti(ti->ti_prev);
	field = &tt->tt_field_thunkoff[ti->ti_fieldidx];
	len = field->tf_typesize(ti_to_thunk(ti->ti_prev));
#ifdef PRINT_BITS
	printf("%s.%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bits)",
		tt->tt_name,
		field->tf_fieldname,
		ti->ti_td.td_diskoff,
		ti->ti_td.td_diskoff + len,
		len);
#else
	printf("%s.%s@%"PRIu64"--%"PRIu64" (%"PRIu64" bytse)",
		tt->tt_name,
		field->tf_fieldname,
		ti->ti_td.td_diskoff/8,
		(ti->ti_td.td_diskoff + len)/8,
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

		vt = &tt->tt_virt[i];
		vt_min = vt->vt_min(ti_to_thunk(ti));
		vt_max = vt->vt_max(ti_to_thunk(ti));
		if (vt_min > vt_max)
			continue;

		if (none_seen) {
			printf("Virtuals: \n");
			none_seen = false;
		}

		printf("%02d. (%s->%s)\n",
			tt->tt_fieldall_c + tt->tt_pointsto_c + i,
			tt_by_num(vt->vt_type_src)->tt_name,
			tt_by_num(vt->vt_type_virttype)->tt_name);
	}
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

		pt = &tt->tt_pointsto[i];
		pt_min = pt->pt_min(ti_to_thunk(ti));
		pt_max = pt->pt_max(ti_to_thunk(ti));
		if (pt_min > pt_max) {
			/* failed some condition, don't display. */
			continue;
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

	if (ti_typenum(ti) == TYPENUM_INVALID)
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
			is_present = field->tf_cond(ti_to_thunk(ti));

		if (is_present == false)
			continue;

		if (field->tf_typenum != TYPENUM_INVALID)
			printf("%02d. ", i);
		else
			printf("--- ");

		print_field(ti, field);
	}
}


void typeinfo_free(struct type_info* ti)
{
	if (ti->ti_td.td_params != NULL) free(ti->ti_td.td_params);
	if (ti->ti_is_virt == true) fsl_virt_clear();
	free(ti);
}

void typeinfo_dump_data(const struct type_info* ti)
{
	uint64_t			type_sz;
	unsigned int			i;

	type_sz = tt_by_ti(ti)->tt_size(ti_to_thunk(ti));

	for (i = 0; i < type_sz / 8; i++) {
		uint8_t	c;

		if ((i % 0x18) == 0) {
			if (i != 0) printf("\n");
			printf("%04x: ", i);
		}
		c = __getLocal(ti->ti_td.td_diskoff + (i*8), 8);
		printf("%c%c ", hexmap[(c >> 4) & 0xf], hexmap[c & 0xf]);

	}

	printf("\n");
	if (type_sz % 8)
		printf("OOPS-- size is not byte-aligned\n");
}

static struct type_info* typeinfo_alloc_generic(
	const struct type_desc	*ti_td,
	const struct type_info	*ti_prev)
{
	struct fsl_rt_table_type	*tt;
	struct type_info		*ret;

	ret = malloc(sizeof(struct type_info));
	memcpy(&ret->ti_td, ti_td, sizeof(*ti_td));

	tt = tt_by_num(ti_typenum(ret));
	if (tt->tt_param_c > 0) {
		unsigned int	len;
		assert (ti_td->td_params != NULL);

		len = sizeof(uint64_t) * tt->tt_param_c;
		ret->ti_td.td_params = malloc(len);
		memcpy(ret->ti_td.td_params, ti_td->td_params, len);
	} else
		ret->ti_td.td_params = NULL;

	ret->ti_prev = ti_prev;
	if (ti_prev != NULL) {
		ret->ti_depth = ti_prev->ti_depth + 1;
	}

	return ret;
}

struct type_info* typeinfo_alloc_pointsto(
	const struct type_desc *ti_td,
	unsigned int		ti_pointsto_idx,
	unsigned int		ti_pointsto_elem,
	const struct type_info*	ti_prev)
{
	struct type_info*		ret;

	ret = typeinfo_alloc_generic(ti_td, ti_prev);
	if (ret == NULL)
		return NULL;

	ret->ti_is_virt = false;
	ret->ti_pointsto = true;
	ret->ti_pointstoidx = ti_pointsto_idx;
	ret->ti_pointsto_elem = ti_pointsto_elem;

	typeinfo_set_dyn(ret);
	if (verify_asserts(ret) == false) {
		typeinfo_free(ret);
		return NULL;
	}

	return ret;
}

struct type_info* typeinfo_alloc(
	const struct type_desc* ti_td,
	unsigned int		ti_fieldidx,
	const struct type_info*	ti_prev)
{
	struct type_info*	ret;

	ret = typeinfo_alloc_generic(ti_td, ti_prev);
	if (ret == NULL)
		return NULL;

	ret->ti_is_virt = false;
	ret->ti_pointsto = false;
	ret->ti_fieldidx = ti_fieldidx;

	typeinfo_set_dyn(ret);
	if (verify_asserts(ret) == false) {
		typeinfo_free(ret);
		return NULL;
	}

	return ret;
}

struct type_info* typeinfo_alloc_virt(
	struct fsl_rt_table_virt* virt,
	const struct type_info*	ti_prev)
{
	struct type_desc	td;
	struct type_info*	ret;

	assert (tt_by_num(virt->vt_type_virttype)->tt_param_c == 0);

	td.td_typenum = virt->vt_type_virttype;
	td.td_diskoff = 0;
	td.td_params = NULL;
	ret = typeinfo_alloc_generic(&td, ti_prev);
	if (ret == NULL)
		return NULL;

	ret->ti_is_virt = true;
	ret->ti_pointsto = false;
	ret->ti_virttype_c = 0;	/* XXX */

	fsl_virt_set(td_explode(&ti_prev->ti_td), virt);

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

	__setDyn(td_explode(&ti->ti_td));

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_field_c; i++) {
		struct fsl_rt_table_field	*field;
		unsigned int			param_c;
		diskoff_t			diskoff;

		field = &tt->tt_field_thunkoff[i];
		if (field->tf_typenum == TYPENUM_INVALID)
			continue;

		param_c = tt_by_num(field->tf_typenum)->tt_param_c;
		uint64_t	params[param_c];

		diskoff = field->tf_fieldbitoff(ti_to_thunk(ti));
		field->tf_params(ti_to_thunk(ti), params);
		__setDyn(field->tf_typenum, diskoff, params);
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
