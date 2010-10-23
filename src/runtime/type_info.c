//#define DEBUG_TYPEINFO
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "debug.h"
#include "type_info.h"

static bool ti_has_loop(const struct type_info* chain);
static struct type_info* typeinfo_alloc_generic(
	const struct type_desc	*ti_td,
	const struct type_info	*ti_prev);
static bool typeinfo_verify_asserts(const struct type_info *ti);
static bool typeinfo_verify(const struct type_info* ti);


void typeinfo_free(struct type_info* ti)
{
	if (ti->ti_virt && ti_xlate(ti) != NULL)
		fsl_virt_free(ti_xlate(ti));
	if (ti_params(ti) != NULL)
		free(ti->ti_td.td_clo.clo_params);
	free(ti);
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

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_TYPEINFO_ALLOC);

	ret = malloc(sizeof(struct type_info));
	memset(ret, 0, sizeof(*ret));

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

static bool typeinfo_verify_asserts(const struct type_info *ti)
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
			DEBUG_TYPEINFO_WRITE(
				"%s: Assert #%d failed!!!\n",
				tt->tt_name, i);
			return false;
		}
	}

	return true;
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
	if (typeinfo_verify_asserts(ti) == false) {
		DEBUG_TYPEINFO_LEAVE();
		return false;
	}

	DEBUG_TYPEINFO_LEAVE();
	return true;
}

struct type_info* typeinfo_alloc_pointsto(
	const struct type_desc			*ti_td,
	const struct fsl_rt_table_pointsto	*ti_pointsto,
	unsigned int				ti_pointsto_elem,
	const struct type_info			*ti_prev)
{
	struct type_info*		ret;

	ret = typeinfo_alloc_generic(ti_td, ti_prev);
	if (ret == NULL)
		return NULL;

	ret->ti_points = ti_pointsto;
	ret->ti_print_name = "points-to";
	if (ti_pointsto->pt_name) ret->ti_print_name = ti_pointsto->pt_name;
	ret->ti_print_idxval = ti_pointsto_elem;

	typeinfo_set_dyn(ret);
	if (typeinfo_verify(ret) == false) {
		typeinfo_free(ret);
		return NULL;
	}

	return ret;
}

struct type_info* typeinfo_alloc_by_field(
	const struct type_desc		*ti_td,
	const struct fsl_rt_table_field	*ti_field,
	const struct type_info		*ti_prev)
{
	struct type_info*	ret;

	DEBUG_TYPEINFO_ENTER();

	DEBUG_TYPEINFO_WRITE("Filling out generic details");

	ret = typeinfo_alloc_generic(ti_td, ti_prev);
	if (ret == NULL) {
		DEBUG_TYPEINFO_LEAVE();
		return NULL;
	}

	ret->ti_field = ti_field;
	ret->ti_print_name = (ti_field) ? ti_field->tf_fieldname : "disk";
	ret->ti_print_idxval = TI_INVALID_IDXVAL;

	DEBUG_TYPEINFO_WRITE("Setting Dyn");

	typeinfo_set_dyn(ret);

	DEBUG_TYPEINFO_WRITE("Verifying");

	if (typeinfo_verify(ret) == false) {
		typeinfo_free(ret);
		DEBUG_TYPEINFO_LEAVE();
		return NULL;
	}

	DEBUG_TYPEINFO_LEAVE();
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

	ret->ti_virt = virt;
	ret->ti_print_name = (virt->vt_name) ? virt->vt_name : "virt";
	ret->ti_print_idxval = idx;


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

/* move typeinfo to next virtual type */
struct type_info* typeinfo_virt_next(struct type_info* ti, int* err_code)
{
	unsigned int	new_idx;
	diskoff_t	new_off;

	assert (ti != NULL);
	assert (ti_xlate(ti) != NULL);
	assert (ti->ti_virt != NULL);

	DEBUG_TYPEINFO_ENTER();

	new_idx = ++ti->ti_print_idxval;
	DEBUG_TYPEINFO_WRITE("new_idx = %d", new_idx);

	/* XXX get_nth is slow, need better interface */
	new_off = fsl_virt_get_nth(ti_xlate(ti), new_idx);
	assert (new_off != 0 && "Next type must be at non-zero voff");
	if (new_off == OFFSET_INVALID) {
		set_err_code(err_code, TI_ERR_BADIDX);
		typeinfo_free(ti);
		ti = NULL;
		goto done;
	}

	/* offset bump */
	ti_offset(ti) = new_off;

done:
	DEBUG_TYPEINFO_LEAVE();

	return ti;
}

void typeinfo_set_dyn(const struct type_info* ti)
{
	TI_INTO_CLO			(ti);
	struct fsl_rt_table_type	*tt;
	unsigned int			i;

	DEBUG_TYPEINFO_ENTER();

	__setDyn(td_explode(&ti->ti_td));

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_fieldstrong_c; i++) {
		struct fsl_rt_table_field	*field;
		unsigned int			param_c;
		diskoff_t			diskoff;

		field = &tt->tt_fieldstrong_table[i];
		if (field->tf_typenum == TYPENUM_INVALID)
			continue;

		DEBUG_TYPEINFO_WRITE("Processing field: %s (type=%s)",
			field->tf_fieldname,
			tt_by_num(field->tf_typenum)->tt_name);

		param_c = tt_by_num(field->tf_typenum)->tt_param_c;
		uint64_t	params[param_c];

		DEBUG_TYPEINFO_WRITE("Reading diskoff");
		diskoff = field->tf_fieldbitoff(clo);
		DEBUG_TYPEINFO_WRITE("Got diskoff: %"PRIu64" bytes", diskoff/8);

		DEBUG_TYPEINFO_WRITE("Getting params");
		field->tf_params(clo, 0, params);
		DEBUG_TYPEINFO_WRITE("NEW_VCLO");
		NEW_VCLO		(new_clo,
					 diskoff, params, ti_xlate(ti));

		__setDyn(field->tf_typenum, &new_clo);
	}

	DEBUG_TYPEINFO_LEAVE();
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

typesize_t ti_size(const struct type_info* ti)
{
	typesize_t ret;

	assert (ti_typenum(ti) != TYPENUM_INVALID);

	ret = tt_by_ti(ti)->tt_size(&ti->ti_td.td_clo);

	return ret;
}
