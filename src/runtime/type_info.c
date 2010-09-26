#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "debug.h"
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
		TI_INTO_CLO(ti);

		as = &tt->tt_assert[i];
		if (as->as_assertf(clo) == false) {
//			printf("!!! !!!Assert #%d failed on type %s!!! !!!\n",
//				i, tt->tt_name);
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
	TI_INTO_CLO(ti);

	num_elems = field->tf_elemcount(clo);
	printf("%s", field->tf_fieldname);
	if (num_elems > 1) {
		printf("[%"PRIu64"]", num_elems);
	}

	field_off = field->tf_fieldbitoff(clo);

	field_typenum = field->tf_typenum;
	if (num_elems > 1 && field_typenum != TYPENUM_INVALID && !field->tf_constsize) {
		unsigned int param_c = tt_by_num(field->tf_typenum)->tt_param_c;
		uint64_t field_params[param_c];
		NEW_CLO(field_closure, field_off, field_params);

		field->tf_params(clo, field_params);

		/* non-constant width.. */
		field_sz = __computeArrayBits(
			field_typenum, &field_closure, num_elems);
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
	field = &tt->tt_field_thunkoff[ti->ti_fieldidx];
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


void typeinfo_free(struct type_info* ti)
{
	if (ti->ti_is_virt && ti_xlate(ti) != NULL)
		fsl_virt_free(ti_xlate(ti));
	if (ti_params(ti) != NULL)
		free(ti->ti_td.td_clo.clo_params);
	free(ti);
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

static bool ti_has_loop(const struct type_info* chain)
{
	const struct type_info	*cur;
	poff_t			top_poff;
	typenum_t		top_typenum;

	cur = chain;
	assert (cur != NULL);

	if (cur->ti_prev == NULL) return false;

	DEBUG_TYPEINFO_WRITE("COMPUTING PHYSOFF %p", ti_xlate(cur));
	top_poff = ti_phys_offset(cur);
	top_typenum = ti_typenum(cur);
	cur = cur->ti_prev;
	DEBUG_TYPEINFO_WRITE("TI_HAS_LOOP: LOOP");

	for (; cur != NULL; cur = cur->ti_prev) {
		/* TODO: cache physical offsets? */
		if (ti_phys_offset(cur) == top_poff &&
		    ti_typenum(cur) == top_typenum)
		{
			return true;
		}
	}

	return false;
}

static struct type_info* typeinfo_alloc_generic(
	const struct type_desc	*ti_td,
	const struct type_info	*ti_prev)
{
	struct fsl_rt_table_type	*tt;
	struct type_info		*ret;

	ret = malloc(sizeof(struct type_info));

	/* set typenum */
	ti_typenum(ret) = td_typenum(ti_td);
	/* set offset */
	ti_offset(ret) = td_offset(ti_td);
	/* set params */
	tt = tt_by_num(ti_typenum(ret));
	if (tt->tt_param_c > 0) {
		unsigned int	len;
		assert (ti_td->td_clo.clo_params != NULL);

		len = sizeof(uint64_t) * tt->tt_param_c;
		ti_params(ret) = malloc(len);
		memcpy(ti_params(ret), ti_td->td_clo.clo_params, len);
	} else
		ti_params(ret) = NULL;

	/* set xlate -- if this is a virt type, its xlate is already
	 * allocated by typeinfo_alloc_virt.. */
	ret->ti_td.td_clo.clo_xlate = td_xlate(ti_td);

	ret->ti_prev = ti_prev;
	ret->ti_depth = (ti_prev != NULL) ? ti_prev->ti_depth + 1 : 0;

	return ret;
}

static bool typeinfo_verify(const struct type_info* ti)
{
	DEBUG_TYPEINFO_ENTER();
	DEBUG_TYPEINFO_WRITE("check loop %p", ti_xlate(ti));
	if (ti_has_loop(ti) == true) {
		DEBUG_TYPEINFO_WRITE("XXX addr already in chain! voff=%"PRIu64
			" poff=%"PRIu64" (%s). xlate=%p",
			ti_offset(ti),
			ti_phys_offset(ti),
			ti_type_name(ti),
			ti->ti_td.td_clo.clo_xlate);
		DEBUG_TYPEINFO_LEAVE();
		return false;
	}

	DEBUG_TYPEINFO_WRITE("verify: check asserts %p", ti_xlate(ti));
	if (verify_asserts(ti) == false) {
		DEBUG_TYPEINFO_LEAVE();
		return false;
	}

	DEBUG_TYPEINFO_LEAVE();
	return true;
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
	if (typeinfo_verify(ret) == false) {
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
	if (typeinfo_verify(ret) == false) {
		typeinfo_free(ret);
		return NULL;
	}

	return ret;
}

#define set_err_code(x,y)		if ((x) != NULL) *(x) = y

struct type_info* typeinfo_alloc_virt_idx(
	struct fsl_rt_table_virt* virt,
	struct type_info*	ti_prev,
	unsigned int		idx,
	int			*err_code)
{
	struct type_desc	td;
	struct type_info*	ret;
	typesize_t		array_bit_off;

	DEBUG_TYPEINFO_ENTER();

	assert (tt_by_num(virt->vt_type_virttype)->tt_param_c == 0 &&
		"Parameterized virtual types not yet supported");

	set_err_code(err_code, TI_ERR_OK);

	DEBUG_TYPEINFO_WRITE("td_vinit");
	td_vinit(&td,
		virt->vt_type_virttype, 0, NULL,
		fsl_virt_alloc(&ti_to_td(ti_prev)->td_clo, virt));

	if (td_xlate(&td) == NULL) {
		/* could not allocate virt */
		set_err_code(err_code, TI_ERR_BADVIRT);
		ret = NULL;
		goto done;
	}

	if (idx != 0) {
		DEBUG_TYPEINFO_WRITE("idx = %d", idx);

		array_bit_off = fsl_virt_get_nth(td_xlate(&td), idx);
		assert (array_bit_off != 0 && "Type must be at non-zero voff");
		if (array_bit_off == OFFSET_INVALID) {
			fsl_virt_free(td_xlate(&td));
			set_err_code(err_code, TI_ERR_BADIDX);
			ret = NULL;
			goto done;
		}

		td_offset(&td) = array_bit_off;
		DEBUG_TYPEINFO_WRITE("idx = %d. Done. voff=%"PRIu64, idx, array_bit_off);
	}

	DEBUG_TYPEINFO_WRITE("alloc_gen");
	assert (td_xlate(&td) != NULL);
	ret = typeinfo_alloc_generic(&td, ti_prev);
	if (ret == NULL) {
		set_err_code(err_code, TI_ERR_BADALLOC);
		ret = NULL;
		goto done;
	}

	assert (ti_xlate(ret) != NULL);

	ret->ti_is_virt = true;
	ret->ti_pointsto = false;
	ret->ti_virttype_c = 0;	/* XXX */

	DEBUG_TYPEINFO_WRITE("now set_dyns");
	typeinfo_set_dyn(ret);
	DEBUG_TYPEINFO_WRITE("virt_alloc_idx: typeinfo_verify");
	if (typeinfo_verify(ret) == false) {
		DEBUG_TYPEINFO_WRITE("virt_alloc_idx: typeinfo_verify BAD!\n");
		typeinfo_free(ret);
		set_err_code(err_code, TI_ERR_BADVERIFY);
		ret = NULL;
		goto done;
	}
	DEBUG_TYPEINFO_WRITE("virt_alloc_idx: typeinfo_verify OK!");

done:
	DEBUG_TYPEINFO_LEAVE();
	return ret;
}

void typeinfo_set_dyn(const struct type_info* ti)
{
	TI_INTO_CLO			(ti);
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

		diskoff = field->tf_fieldbitoff(clo);
		field->tf_params(clo, params);

		NEW_VCLO		(new_clo,
					 diskoff, params, ti_xlate(ti));

		__setDyn(field->tf_typenum, &new_clo);
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

diskoff_t ti_phys_offset(const struct type_info* ti)
{
	diskoff_t	voff;
	diskoff_t	off;

	assert (ti != NULL);

	voff = ti_offset(ti);
	if (ti->ti_td.td_clo.clo_xlate == NULL) {
		/* no xlate, phys = virt */
		return voff;
	}

	DEBUG_TYPEINFO_WRITE("enter xlate %p", ti_xlate(ti));
	off = fsl_virt_xlate(&ti->ti_td.td_clo, voff);
	DEBUG_TYPEINFO_WRITE("leave xlate %p", ti_xlate(ti));

	return off;
}
