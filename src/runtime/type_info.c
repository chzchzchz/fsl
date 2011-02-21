//#define DEBUG_TYPEINFO
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include "alloc.h"
#include "debug.h"
#include "type_info.h"
#include "lookup.h"
#include "io.h"

static bool ti_has_loop(const struct type_info* chain);
static struct type_info* typeinfo_alloc_generic(
	const struct type_desc	*ti_td,
	struct type_info	*ti_prev);
static bool typeinfo_verify_asserts(const struct type_info *ti);
static bool typeinfo_verify(const struct type_info* ti);
static unsigned int typeinfo_unref(struct type_info* ti);
static struct type_info* typeinfo_follow_name(
	struct type_info*	ti, /* array to avoid aliasing */
	const char*		fieldname);

void typeinfo_free(struct type_info* ti)
{
	if (typeinfo_unref(ti)) return;

	if (ti->ti_virt && ti_xlate(ti) != NULL) {
		fsl_virt_unref(&ti_clo(ti));
		ti_xlate(ti) = NULL;
	}
	if (ti_params(ti) != NULL)
		fsl_free(ti->ti_td.td_clo.clo_params);

	if (ti->ti_prev != NULL) typeinfo_free(ti->ti_prev);

	fsl_free(ti);
}

static bool ti_has_loop(const struct type_info* chain)
{
	const struct type_info	*cur;
	poff_t			top_poff;
	typenum_t		top_typenum;

	cur = chain;
	FSL_ASSERT (cur != NULL);

	if (cur->ti_prev == NULL) return false;

	top_poff = ti_phys_offset(cur);
	top_typenum = ti_typenum(cur);
	cur = cur->ti_prev;

	for (; cur != NULL; cur = cur->ti_prev) {
		/* TODO: cache physical offsets? */
		if (ti_phys_offset(cur) == top_poff &&
		    ti_typenum(cur) == top_typenum)
		{
			DEBUG_TYPEINFO_WRITE("TI_HAS_LOOP: LOOP! BAIL");
			return true;
		}
	}

	return false;
}

static struct type_info* typeinfo_alloc_generic(
	const struct type_desc	*ti_td,
	struct type_info	*ti_prev)
{
	const struct fsl_rtt_type	*tt;
	struct type_info		*ret;

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_TYPEINFO_ALLOC);

	if (offset_in_range(td_offset(ti_td)) == false) {
		DEBUG_TYPEINFO_WRITE("Bad range %"PRIu64,
			td_offset(ti_td));
		return NULL;
	}

	ret = fsl_alloc(sizeof(struct type_info));
	memset(ret, 0, sizeof(*ret));

	/* set typenum */
	ti_typenum(ret) = td_typenum(ti_td);
	/* set offset */
	FSL_ASSERT (td_offset(ti_td) < __FROM_OS_BDEV_BYTES*8);
	ti_offset(ret) = td_offset(ti_td);
	/* set params */
	tt = tt_by_num(ti_typenum(ret));
	if (	ti_typenum(ret) == TYPENUM_INVALID ||
		(tt = tt_by_ti(ret))->tt_param_c == 0)
	{
		ti_params(ret) = NULL;
	} else {
		unsigned int	len;
		FSL_ASSERT (ti_td->td_clo.clo_params != NULL);

		len = sizeof(uint64_t) * tt->tt_param_c;
		ti_params(ret) = fsl_alloc(len);
		memcpy(ti_params(ret), ti_td->td_clo.clo_params, len);
	}

	ret->ti_prev = ti_prev;
	ret->ti_depth = (ti_prev != NULL) ? ti_prev->ti_depth + 1 : 0;

	/* set xlate -- if this is a virt type, its xlate is already
	 * allocated by typeinfo_alloc_virt.. */
	ret->ti_td.td_clo.clo_xlate = td_xlate(ti_td);

	typeinfo_ref(ret);
	if (ti_prev != NULL) typeinfo_ref(ti_prev);

	return ret;
}

static bool typeinfo_verify_asserts(const struct type_info *ti)
{
	const struct fsl_rtt_type	*tt;
	unsigned int			i;

	FSL_ASSERT (ti != NULL);

	if (ti_typenum(ti) == TYPENUM_INVALID) return true;

	tt = tt_by_ti(ti);
	for (i = 0; i < tt->tt_assert_c; i++) {
		const struct fsl_rtt_assert	*as;
		TI_INTO_CLO(ti);

		as = &tt->tt_assert[i];
		DEBUG_TYPEINFO_WRITE("check assertions %d in %s", i, tt->tt_name);
		if (as->as_assertf(clo) == false) {
			DEBUG_TYPEINFO_WRITE(
				"%s: Assert #%d failed!!!\n",
				tt->tt_name, i);
			fsl_env->fctx_failed_assert = as->as_name;
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
		DEBUG_TYPEINFO_WRITE("verify: addr already in chain! voff=%"PRIu64
			" poff=%"PRIu64" (%s). xlate=%p",
			ti_offset(ti),
			ti_phys_offset(ti),
			ti_type_name(ti),
			ti->ti_td.td_clo.clo_xlate);
		DEBUG_TYPEINFO_LEAVE();
		return false;
	}

	DEBUG_TYPEINFO_WRITE("verify: check asserts. xlate=%p", ti_xlate(ti));
	if (typeinfo_verify_asserts(ti) == false) {
		DEBUG_TYPEINFO_WRITE("verify: FSL_ASSERTions failed");
		DEBUG_TYPEINFO_LEAVE();
		return false;
	}

	DEBUG_TYPEINFO_WRITE("Verify OK");
	DEBUG_TYPEINFO_LEAVE();
	return true;
}

struct type_info* typeinfo_alloc_pointsto(
	const struct type_desc		*ti_td,
	const struct fsl_rtt_pointsto	*ti_pointsto,
	unsigned int			ti_pointsto_elem,
	struct type_info		*ti_prev)
{
	struct type_info*		ret;

	ret = typeinfo_alloc_generic(ti_td, ti_prev);
	if (ret == NULL)
		return NULL;

	ret->ti_points = ti_pointsto;
	ret->ti_print_name = "points-to";
	if (ti_pointsto->pt_name) ret->ti_print_name = ti_pointsto->pt_name;
	ret->ti_print_idxval = ti_pointsto_elem;
	ret->ti_iter = &ti_pointsto->pt_iter;

	if (typeinfo_verify(ret) == false) {
		typeinfo_free(ret);
		return NULL;
	}

	return ret;
}

struct type_info* typeinfo_alloc_iter(
	const struct type_desc		*ti_td,
	const struct fsl_rt_iter	*ti_iter,
	unsigned int			ti_iter_elem,
	struct type_info*		ti_prev)
{
	struct type_info*		ret;

	DEBUG_TYPEINFO_ENTER();

	ret = typeinfo_alloc_generic(ti_td, ti_prev);
	if (ret == NULL) goto done;

	DEBUG_TYPEINFO_WRITE("Generic alloced ptr=%p", ret);

	ret->ti_print_name = "iter";
	ret->ti_print_idxval = ti_iter_elem;
	ret->ti_iter = ti_iter;

	if (typeinfo_verify(ret) == false) {
		typeinfo_free(ret);
		ret = NULL;
		goto done;
	}

done:
	DEBUG_TYPEINFO_LEAVE();
	return ret;
}

struct type_info* typeinfo_alloc_by_field(
	const struct type_desc		*ti_td,
	const struct fsl_rtt_field	*ti_field,
	struct type_info		*ti_prev)
{
	struct type_info	*ret;
	struct fsl_rt_mapping	*rtm;

	DEBUG_TYPEINFO_ENTER();

	DEBUG_TYPEINFO_WRITE("alloc_by_field: td_offset=%"PRIu64, td_offset(ti_td));

	DEBUG_TYPEINFO_WRITE("Filling out generic details");
	ret = typeinfo_alloc_generic(ti_td, ti_prev);
	if (ret == NULL) {
		DEBUG_TYPEINFO_LEAVE();
		return NULL;
	}

	ret->ti_field = ti_field;
	ret->ti_print_name = (ti_field) ? ti_field->tf_fieldname : "disk";
	ret->ti_print_idxval = TI_INVALID_IDXVAL;
	if (ti_prev &&  (rtm = ti_xlate(ti_prev)) != NULL) {
		ret->ti_td.td_clo.clo_xlate = rtm;
		fsl_virt_ref(&ti_clo(ret));
	}

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
	const struct fsl_rtt_virt*	virt,
	struct type_info*		ti_prev,
	unsigned int			idx,
	int*				err_code)
{
	struct type_desc	td;
	struct type_info*	ret;
	typesize_t		array_bit_off;

	DEBUG_TYPEINFO_ENTER();

	FSL_ASSERT (tt_by_num(virt->vt_type_virttype)->tt_param_c == 0 &&
		"Parameterized virtual types not yet supported");

	set_err_code(err_code, TI_ERR_OK);

	DEBUG_TYPEINFO_WRITE("td_vinit parent=%s@voff=%"PRIu64". child=%s. name=%s",
		tt_by_ti(ti_prev)->tt_name,
		ti_offset(ti_prev),
		tt_by_num(virt->vt_type_virttype)->tt_name,
		virt->vt_name);
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
		DEBUG_TYPEINFO_WRITE("alloc_virt_idx: idx = %d", idx);

		array_bit_off = fsl_virt_get_nth(td_xlate(&td), idx);
		FSL_ASSERT (array_bit_off != 0 && "Type must be at non-zero voff");
		if (offset_is_bad(array_bit_off)) {
			fsl_virt_free(td_xlate(&td));
			set_err_code(err_code, TI_ERR_BADIDX);
			ret = NULL;
			goto done;
		}

		td_offset(&td) = array_bit_off;
		DEBUG_TYPEINFO_WRITE("idx = %d. Done. voff=%"PRIu64,
			idx, array_bit_off);
	}

	/* verify we have a valid offset into disk */
	if (!offset_in_range(fsl_virt_xlate_safe(&td.td_clo, td_offset(&td)))) {
		DEBUG_TYPEINFO_WRITE("Bad poff in virt.");
		fsl_virt_free(td_xlate(&td));
		ret = NULL;
		goto done;
	}

	DEBUG_TYPEINFO_WRITE("virt_alloc_idx: alloc_gen voff=%"PRIu64,
		td_offset(&td));
	FSL_ASSERT (td_xlate(&td) != NULL);
	ret = typeinfo_alloc_generic(&td, ti_prev);
	if (ret == NULL) {
		set_err_code(err_code, TI_ERR_BADALLOC);
		ret = NULL;
		goto done;
	}

	DEBUG_TYPEINFO_WRITE("virt_alloc_idx: poff=%"PRIu64,
		ti_phys_offset(ret));

	FSL_ASSERT (ti_xlate(ret) != NULL);

	ret->ti_virt = virt;
	ret->ti_print_name = (virt->vt_name) ? virt->vt_name : "virt";
	ret->ti_print_idxval = idx;
	ret->ti_iter = &virt->vt_iter;

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
	FSL_ASSERT ((ret == NULL || (offset_in_range(ti_phys_offset(ret))))
		&& "Returning with bad offset.");

	DEBUG_TYPEINFO_LEAVE();
	return ret;
}

/* move typeinfo to next virtual type */
struct type_info* typeinfo_virt_next(struct type_info* ti, int* err_code)
{
	unsigned int	new_idx;
	diskoff_t	new_off;

	FSL_ASSERT (ti != NULL);
	FSL_ASSERT (ti_xlate(ti) != NULL);
	FSL_ASSERT (ti->ti_virt != NULL);

	DEBUG_TYPEINFO_ENTER();

	new_idx = ++ti->ti_print_idxval;
	DEBUG_TYPEINFO_WRITE("new_idx = %d", new_idx);

	/* XXX get_nth is slow, need better interface */
	new_off = fsl_virt_get_nth(ti_xlate(ti), new_idx);
	FSL_ASSERT (new_off != 0 && "Next type must be at non-zero voff");
	if (offset_is_bad(new_off)) {
		if (new_off == OFFSET_INVALID) {
			set_err_code(err_code, TI_ERR_BADIDX);
		} else if (new_off == OFFSET_EOF) {
			set_err_code(err_code, TI_ERR_EOF);
		} else {
			FSL_ASSERT(0 == 1 && "UNKNOWN ERROR");
		}
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

diskoff_t ti_phys_offset(const struct type_info* ti)
{
	diskoff_t	voff;
	diskoff_t	off;

	FSL_ASSERT (ti != NULL);

	voff = ti_offset(ti);
	if (ti->ti_td.td_clo.clo_xlate == NULL) {
		/* no xlate, phys = virt */
		return voff;
	}

	off = fsl_virt_xlate(&ti->ti_td.td_clo, voff);

	return off;
}

typesize_t ti_size(const struct type_info* ti)
{
	typesize_t ret;

	if (ti_typenum(ti) == TYPENUM_INVALID) {
		FSL_ASSERT (ti->ti_field != NULL && "Expected field entry");
		FSL_ASSERT (ti->ti_prev != NULL && "No parent?");
		/* TYPENUM_INVALID => constant size => don't need parent */
		ret = ti->ti_field->tf_typesize(NULL);
	} else {
		ret = tt_by_ti(ti)->tt_size(&ti->ti_td.td_clo);
	}

	return ret;
}

struct type_info* typeinfo_follow_field(
	struct type_info* tip,
	const struct fsl_rtt_field* tif)
{
	diskoff_t	off;

	if (tif->tf_cond != NULL)
		if (tif->tf_cond(&ti_clo(tip)) == false)
			return NULL;

	off = tif->tf_fieldbitoff(&ti_clo(tip));
	return typeinfo_follow_field_off(tip, tif, off);
}

struct type_info* typeinfo_follow_field_off_idx(
	struct type_info*		ti_parent,
	const struct fsl_rtt_field*	ti_field,
	size_t				off,
	uint64_t			idx)
{
	struct type_desc		field_td;
	unsigned int			param_c;

	if (ti_field->tf_typenum != TYPENUM_INVALID) {
		const struct fsl_rtt_type	*field_type;
		field_type = tt_by_num(ti_field->tf_typenum);
		param_c = field_type->tt_param_c;
	} else
		param_c = 0;
	uint64_t	parambuf[param_c];

	ti_field->tf_params(&ti_clo(ti_parent), idx, parambuf);
	td_init(&field_td, ti_field->tf_typenum, off, parambuf);

	return typeinfo_alloc_by_field(&field_td, ti_field, ti_parent);
}

struct type_info* typeinfo_follow_pointsto(
	struct type_info*		ti_parent,
	const struct fsl_rtt_pointsto*	ti_pt,
	uint64_t			idx)
{
	struct type_info	*ret;
	struct type_desc	pt_td;
	uint64_t		parambuf[tt_by_ti(ti_parent)->tt_param_c];
	void			*xlate;

	FSL_ASSERT (	ti_pt->pt_iter.it_min(&ti_clo(ti_parent)) <= idx &&
			"IDX TOO SMALL");

	td_vinit(&pt_td,
		ti_pt->pt_iter.it_type_dst,
		ti_pt->pt_iter.it_range(&ti_clo(ti_parent), idx, parambuf, &xlate),
		parambuf,
		xlate);

	DEBUG_TYPEINFO_WRITE("POINTSTO XLATE: %p", td_xlate(&pt_td));

	FSL_ASSERT (offset_is_bad(td_offset(&pt_td)) == false &&
		"VERIFY RANGE CALL.");

	ret = typeinfo_alloc_pointsto(&pt_td, ti_pt, idx, ti_parent);

	return ret;
}

struct type_info* typeinfo_follow_iter(
	struct type_info*		ti_parent,
	const struct fsl_rt_iter*	ti_iter,
	uint64_t			idx)
{
	struct type_info	*ret;
	struct type_desc	iter_td;
	uint64_t		parambuf[tt_by_ti(ti_parent)->tt_param_c];
	void			*xlate;
	diskoff_t		diskoff;

	DEBUG_TYPEINFO_ENTER();

	diskoff = ti_iter->it_range(&ti_clo(ti_parent), idx, parambuf, &xlate);
	td_vinit(&iter_td, ti_iter->it_type_dst, diskoff, parambuf, xlate);

	FSL_ASSERT (offset_is_bad(td_offset(&iter_td)) == false &&
		"VERIFY ITER RANGE CALL.");

	DEBUG_TYPEINFO_WRITE("iter idx=%"PRIu64" / offset: %"PRIu64,
		idx,
		td_offset(&iter_td));

	ret = typeinfo_alloc_iter(&iter_td, ti_iter, idx, ti_parent);

	DEBUG_TYPEINFO_LEAVE();

	return ret;
}

void typeinfo_ref(struct type_info* ti)
{
	ti->ti_ref_c++;
}

static unsigned int typeinfo_unref(struct type_info* ti)
{
	FSL_ASSERT (ti->ti_ref_c > 0);
	ti->ti_ref_c--;
	return ti->ti_ref_c;
}

unsigned int typeinfo_to_buf(
	struct type_info* src, void* buf, unsigned int bytes)
{
	typesize_t	src_sz;
	unsigned int	copy_bytes;
	diskoff_t	base_off;

	src_sz = (ti_size(src)+7)/8;
	copy_bytes = (src_sz < bytes) ? src_sz : bytes;
	base_off = ti_phys_offset(src)/8;
	fsl_io_read_bytes(buf, copy_bytes, base_off);
	return copy_bytes;
}

#define COPY_BUF_SZ	4096

void typeinfo_phys_copy(struct type_info* dst, struct type_info* src)
{
	typesize_t		src_sz, dst_sz;
	diskoff_t		src_base, dst_base;
	unsigned int		xfer_total;
	uint8_t			*buf;

	src_sz = ti_size(src);
	dst_sz = ti_size(dst);

	FSL_ASSERT (src_sz == dst_sz && "Can't copy with different sizes!");
	FSL_ASSERT (src_sz % 8 == 0);

	src_sz /= 8;
	dst_sz /= 8;

	FSL_ASSERT (ti_xlate(dst) == NULL && "VIRT NOT SUPPORTED");
	FSL_ASSERT (ti_xlate(src) == NULL && "VIR TNOT SUPPORTED");

	src_base = ti_offset(src);
	dst_base = ti_offset(dst);
	FSL_ASSERT (src_base % 8 == 0);
	FSL_ASSERT (dst_base % 8 == 0);
	src_base /= 8;
	dst_base /= 8;

	buf = fsl_alloc(COPY_BUF_SZ);
	FSL_ASSERT (buf != NULL);

	xfer_total = 0;

	while (xfer_total < src_sz) {
		unsigned int	to_xfer;
		to_xfer = src_sz - xfer_total;
		if (to_xfer > COPY_BUF_SZ) to_xfer = COPY_BUF_SZ;
		fsl_io_read_bytes(buf, to_xfer, xfer_total + src_base);
		fsl_io_write_bytes(buf, to_xfer, xfer_total + dst_base);
		xfer_total += to_xfer;
	}

	fsl_free(buf);
}

char* typeinfo_getname(struct type_info* ti, char* buf, unsigned int len)
{
	struct type_info	*name_ti;
	unsigned int		read_len;
	char			*ret = NULL;

	DEBUG_TYPEINFO_ENTER();

	if (ti == NULL) goto done;

	name_ti = typeinfo_lookup_follow_direct(ti, "name");
	if (name_ti == NULL) goto done;

	DEBUG_TYPEINFO_WRITE("NAME VOFF=%"PRIu64". POFF=%"PRIu64,
		ti_offset(name_ti) / 8,
		ti_phys_offset(name_ti) / 8);

	ret = buf;
	read_len = typeinfo_to_buf(name_ti, buf, len-1);
	buf[read_len] = '\0';
	typeinfo_free(name_ti);

done:
	DEBUG_TYPEINFO_LEAVE();
	return ret;
}

struct type_info* typeinfo_alloc_origin(void)
{
	struct type_desc	init_td = td_origin();
	return typeinfo_alloc_by_field(&init_td, NULL, NULL);
}

/* BIG NOTE HERE:
 * idx always starts at 0!!
 * This is so that we don't have to write different interfaces depending on
 * whether it is a points-to or an array or whatever.
 * Just [0..(num_elems-1]
 */
struct type_info* typeinfo_lookup_follow_idx_all(
	struct type_info*	ti_parent,
	const char*		fieldname,
	uint64_t		idx,
	bool			accept_names)
{
	const struct fsl_rtt_type	*tt;
	const struct fsl_rtt_field*	tf;
	const struct fsl_rtt_pointsto*	pt;
	const struct fsl_rtt_virt*	vt;

	tt = tt_by_ti(ti_parent);
	tf = fsl_lookup_field(tt, fieldname);
	if (tf != NULL) {
		if (tf->tf_typenum == TYPENUM_INVALID) return NULL;
		if (tf->tf_cond != NULL)
			if (tf->tf_cond(&ti_clo(ti_parent)) == false) {
				DEBUG_TYPEINFO_WRITE("BAD COND");
				return NULL;
			}

		return typeinfo_follow_field_off(
				ti_parent,
				tf,
				ti_field_off(ti_parent, tf, idx));
	}
	pt = fsl_lookup_points(tt, fieldname);
	if (pt != NULL)
		return typeinfo_follow_pointsto(
			ti_parent, pt, idx+pt->pt_iter.it_min(&ti_clo(ti_parent)));
	vt = fsl_lookup_virt(tt, fieldname);
	if (vt != NULL) {
		int	err;
		return typeinfo_follow_virt(
			ti_parent, vt,
			idx+vt->vt_iter.it_min(&ti_clo(ti_parent)),
			&err);
	}

	if (!accept_names) return NULL;

	/* last effort: scan all strong fields for names */
	return typeinfo_follow_name(ti_parent, fieldname);
}

static struct type_info* typeinfo_follow_name(
	struct type_info*	ti, /* array to avoid aliasing */
	const char*		fieldname)
{
	const struct fsl_rtt_pointsto*	pt;
	const struct fsl_rtt_virt*	vt;
	uint64_t			idx;
	struct type_info		*ret, *ti_prev;
	char				*name_buf;

	ti_prev = ti->ti_prev;
	if (ti_prev == NULL) return NULL;

	name_buf = fsl_alloc(256);

	if ((pt = ti->ti_points) != NULL) {
		/* scan all members of points */
		uint64_t max_idx;
		idx = pt->pt_iter.it_min(&ti_clo(ti_prev));
		if (idx == ~0) goto done;
		max_idx = pt->pt_iter.it_max(&ti_clo(ti_prev));
		for (; idx <= max_idx; idx++) {
			ret = typeinfo_follow_pointsto(ti_prev, pt, idx);
			if (ret == NULL) continue;
			if (	typeinfo_getname(ret, name_buf, 255) == NULL ||
				strcmp(name_buf, fieldname) != 0)
			{
				typeinfo_free(ret);
			} else
				goto done;
		}
		ret = NULL;
	} else if ((vt = ti->ti_virt) != NULL) {
		/* scan all virt members */
		int	err;
		idx = vt->vt_iter.it_min(&ti_clo(ti_prev));
		if (idx == ~0) goto done;
		err = 0;
		while (err == TI_ERR_OK || err == TI_ERR_BADVERIFY) {
			ret = typeinfo_follow_virt(ti_prev, vt, idx, &err);
			idx++;
			if (ret == NULL) continue;
			if (	typeinfo_getname(ret, name_buf, 255) == NULL ||
				strcmp(name_buf, fieldname) != 0)
			{
				typeinfo_free(ret);
			} else
				goto done;
		}
		ret = NULL;
	}

done:
	fsl_free(name_buf);
	if (ret != NULL) ret->ti_flag |= TI_FL_FROMNAME;
	return ret;
}

struct type_info* typeinfo_reindex(
	const struct type_info* ti_p, unsigned int idx)
{
	const struct fsl_rtt_pointsto   *pt;
	const struct fsl_rtt_virt       *vt;

	if (ti_p == NULL) return NULL;
	if (ti_p->ti_prev == NULL) return NULL;

	if (ti_p->ti_field != NULL) {
		uint64_t	max_idx;

		max_idx = ti_p->ti_field->tf_elemcount(&ti_clo(ti_p->ti_prev));
		/* exceeded number of elements */
		if (max_idx <= idx) return NULL;

		return typeinfo_follow_field_off_idx(
			ti_p->ti_prev, ti_p->ti_field,
			ti_field_off(ti_p->ti_prev, ti_p->ti_field, idx),
			idx);
	} else if ((pt = ti_p->ti_points) != NULL) {
		uint64_t		it_min;
		it_min = pt->pt_iter.it_min(&ti_clo(ti_p->ti_prev));
		if (it_min == ~0) return NULL;
		return typeinfo_follow_pointsto(ti_p->ti_prev, pt, idx+it_min);
	} else if ((vt = ti_p->ti_virt) != NULL) {
		int		err;
		uint64_t		it_min;
		it_min = vt->vt_iter.it_min(&ti_clo(ti_p->ti_prev));
		if (it_min == ~0) return NULL;
		return typeinfo_follow_virt(ti_p->ti_prev, vt, idx+it_min, &err);
	}

	return NULL;
}

typesize_t ti_field_size(
	const struct type_info* ti, const struct fsl_rtt_field* tf,
	uint64_t* num_elems)
{
	typenum_t	field_typenum;
	typesize_t	field_sz;
	TI_INTO_CLO(ti);

	DEBUG_TYPEINFO_WRITE("Computing elems.");
	*num_elems = tf->tf_elemcount(clo);

	/* compute field width */
	DEBUG_TYPEINFO_WRITE("Computing width.");
	field_typenum = tf->tf_typenum;
	if (*num_elems > 1 &&
	    field_typenum != TYPENUM_INVALID &&
	    ((tf->tf_flags & FIELD_FL_CONSTSIZE) == 0 &&
	     (tf->tf_flags & FIELD_FL_FIXED) == 0))
	{
		/* non-constant width.. */
		field_sz = __computeArrayBits(
			ti_typenum(ti), clo, tf->tf_fieldnum, *num_elems);
	} else {
		/* constant width */
		field_sz = tf->tf_typesize(clo);
		field_sz *= *num_elems;
	}

	DEBUG_TYPEINFO_WRITE("Field size = %"PRIu64, field_sz);

	return field_sz;
}

typesize_t ti_field_off(
	const struct type_info* ti, const struct fsl_rtt_field* tf,
	unsigned int idx)
{
	typenum_t	field_typenum;
	typesize_t	field_sz;
	TI_INTO_CLO(ti);

	/* compute field width */
	DEBUG_TYPEINFO_WRITE("Computing width.");
	field_typenum = tf->tf_typenum;
	if (idx > 0 &&
	    field_typenum != TYPENUM_INVALID &&
	    ((tf->tf_flags & FIELD_FL_CONSTSIZE) == 0 &&
	     (tf->tf_flags & FIELD_FL_FIXED) == 0))
	{
		/* non-constant width.. */
		field_sz = __computeArrayBits(
			ti_typenum(ti), clo, tf->tf_fieldnum, idx);
	} else if (idx > 0) {
		/* constant width */
		field_sz = tf->tf_typesize(clo);
		field_sz *= idx;
	} else
		field_sz = 0;

	DEBUG_TYPEINFO_WRITE("Field size = %"PRIu64, field_sz);

	return field_sz+tf->tf_fieldbitoff(clo);
}
