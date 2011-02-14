//#define DEBUG_VIRT
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "runtime.h"
#include "virt.h"
#include "alloc.h"
#include "debug.h"

static bool fsl_virt_load_cache(struct fsl_rt_mapping* rtm, bool no_verify);
static void fsl_virt_cache_update(
	struct fsl_rt_mapping* rtm, int idx, uint64_t v);
static diskoff_t fsl_virt_cache_find(const struct fsl_rt_mapping* rtm, int idx);
static void fsl_virt_init_cache(struct fsl_rt_mapping* rtm);
static diskoff_t fsl_virt_xlate_miss(struct fsl_rt_mapping *rtm, int idx);

uint64_t fsl_virt_xlate_safe(
	const struct fsl_rt_closure* clo, uint64_t bit_voff)
{
	uint64_t	ret;

	FSL_ASSERT (fsl_env->fctx_except.ex_in_unsafe_op == false);

	fsl_env->fctx_except.ex_err_unsafe_op = 0;

	fsl_env->fctx_except.ex_in_unsafe_op = true;
	ret = fsl_virt_xlate(clo, bit_voff);
	fsl_env->fctx_except.ex_in_unsafe_op = false;

	if (fsl_env->fctx_except.ex_err_unsafe_op != 0) {
		DEBUG_VIRT_WRITE("xlate_safe: bad ret on setjmp");
		fsl_env->fctx_except.ex_err_unsafe_op = 0;
		return OFFSET_INVALID;
	}

	return ret;
}

static diskoff_t fsl_virt_xlate_miss(struct fsl_rt_mapping *rtm, int idx)
{
	diskoff_t		base;
	uint64_t params[tt_by_num(rtm->rtm_virt->vt_iter.it_type_dst)->tt_param_c];

	DEBUG_VIRT_WRITE("&rtm->rtm_clo = %p", rtm->rtm_clo);
	DEBUG_VIRT_WRITE("rtm_clo->clo_offset: %"PRIu64" bits (%"PRIu64" bytes)",
		rtm->rtm_clo->clo_offset,
		rtm->rtm_clo->clo_offset / 8);
	base = rtm->rtm_virt->vt_iter.it_range(rtm->rtm_clo, idx, params);
	DEBUG_VIRT_WRITE("BASE FOUND: %"PRIu64, base);

	return base;
}

static uint64_t fsl_virt_xlate_rtm(struct fsl_rt_mapping* rtm, uint64_t bit_off)
{
	uint64_t	total_bits, idx, base, off;

	DEBUG_VIRT_ENTER();

	FSL_ASSERT (rtm != NULL && "No xlate.");
	FSL_ASSERT (rtm->rtm_clo != NULL && "xlate not fully initialized?");
	DEBUG_VIRT_WRITE("virt bit_off=%"PRIu64, bit_off);
	FSL_ASSERT ((bit_off % 8) == 0 && "Miss aligned xlate");

	total_bits = fsl_virt_total_bits(rtm);
	DEBUG_VIRT_WRITE("virt total bits=%"PRIu64, total_bits);

	if (bit_off >= total_bits && fsl_env->fctx_except.ex_in_unsafe_op) {
		DEBUG_VIRT_WRITE("Taking longjmp");
		fsl_env->fctx_except.ex_err_unsafe_op = 1;
		DEBUG_VIRT_LEAVE();
		return ~0;
	}

	FSL_ASSERT (bit_off < total_bits && "Accessing past virt bits");
	DEBUG_VIRT_WRITE("rtm->rtm_cached_min = %"PRIu64, rtm->rtm_cached_minidx);
	DEBUG_VIRT_WRITE("rtm->rtm_cached_max = %"PRIu64, rtm->rtm_cached_maxidx);

	idx = bit_off / rtm->rtm_cached_srcsz;
	idx += rtm->rtm_cached_minidx;	/* base. */

	base = fsl_virt_cache_find(rtm, idx);
	if (base == ~0) {
		base = fsl_virt_xlate_miss(rtm, idx);
		DEBUG_VIRT_WRITE("WANTED BITOFF: %"PRIu64, bit_off);

		fsl_virt_cache_update(rtm, idx, base);
	} else {
		FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_XLATE_HIT);
	}
	off = bit_off % rtm->rtm_cached_srcsz;

	FSL_ASSERT (bit_off != base+off &&
		"Base+Off=BitOff? => Identity xlate. Probably wrong.");
	FSL_ASSERT ((base+off) != 0 && "xlated addr == origin? Probably wrong.");

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_XLATE_CALL);

	DEBUG_VIRT_LEAVE();

	return base + off;
}

uint64_t fsl_virt_xlate(const struct fsl_rt_closure* clo, uint64_t bit_off)
{
	struct fsl_rt_mapping	*rtm;
	uint64_t		ret;

	DEBUG_VIRT_ENTER();

	DEBUG_VIRT_WRITE("xlating!!!");

	FSL_ASSERT (clo != NULL);

	rtm = clo->clo_xlate;
	DEBUG_VIRT_WRITE("closure offset=%"PRIu64, clo->clo_offset);
	DEBUG_VIRT_WRITE("&rtm = %p", rtm);
	ret = fsl_virt_xlate_rtm(rtm, bit_off);

	DEBUG_VIRT_LEAVE();
	return ret;
}

static void fsl_virt_ref_all(struct fsl_rt_closure* clo)
{
	if (clo == NULL) return;
	if (clo->clo_xlate == NULL) return;
	fsl_virt_ref(clo);
	fsl_virt_ref_all(clo->clo_xlate->rtm_clo);
}

static void fsl_virt_unref_all(struct fsl_rt_closure* clo)
{
	if (clo == NULL) return;
	if (clo->clo_xlate == NULL) return;
	fsl_virt_unref(clo);
	fsl_virt_unref_all(clo->clo_xlate->rtm_clo);
}

void fsl_virt_free(struct fsl_rt_mapping* rtm)
{
	DEBUG_VIRT_ENTER();
	FSL_ASSERT (rtm != NULL);
	FSL_ASSERT (rtm->rtm_ref_c == 1);
	DEBUG_VIRT_WRITE("Freeing: rtm=%p", rtm);
	fsl_virt_unref_all(rtm->rtm_clo);
	fsl_free(rtm);
	DEBUG_VIRT_LEAVE();
}

void fsl_virt_ref(struct fsl_rt_closure* clo)
{
	struct fsl_rt_mapping	*rtm;

	rtm = clo->clo_xlate;
	if (rtm == NULL) return;

	rtm->rtm_ref_c++;
}

void fsl_virt_unref(struct fsl_rt_closure* clo)
{
	struct fsl_rt_mapping	*rtm;

	rtm = clo->clo_xlate;
	if (rtm == NULL) return;

	if (rtm->rtm_ref_c == 1)
		fsl_virt_free(rtm);
	else
		rtm->rtm_ref_c--;
}

struct fsl_rt_mapping*  fsl_virt_alloc(
	struct fsl_rt_closure* parent,
	const struct fsl_rtt_virt* vt)
{
	struct fsl_rt_mapping		*rtm;

	FSL_ASSERT (parent != NULL);
	FSL_ASSERT (vt != NULL);

	DEBUG_VIRT_ENTER();
	DEBUG_VIRT_WRITE("COPYING FOR VIRT_ALLOC");

	rtm = fsl_alloc(sizeof(*rtm));
	rtm->rtm_virt = vt;
	rtm->rtm_clo = parent;
	fsl_virt_ref_all(parent);
	rtm->rtm_ref_c = 1;

	DEBUG_VIRT_WRITE("DYNALLOC: rtm=%p. rtm->rtm_clo=%p",
		rtm, rtm->rtm_clo);

	/* XXX for the time being, assume all source types are same size */
	if (fsl_virt_load_cache(rtm, true) == false) {
		/* empty type */
		fsl_virt_free(rtm);
		rtm = NULL;
		goto done;
	}

	fsl_virt_init_cache(rtm);

	/* make sure we don't allocate a loop */
	if (parent->clo_xlate != NULL) {
		if (fsl_virt_xlate(parent, 0) == fsl_virt_xlate_rtm(rtm, 0)) {
			DEBUG_VIRT_WRITE("Loop. Do not allocate");
			fsl_virt_free(rtm);
			rtm = NULL;
			goto done;
		}
	}
done:
	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_XLATE_ALLOC);

	DEBUG_VIRT_WRITE("fresh new RTM %p", rtm);
	DEBUG_VIRT_LEAVE();
	return rtm;
}

/* sets up some cached values */
static bool fsl_virt_load_cache(struct fsl_rt_mapping* rtm, bool no_verify)
{
	const struct fsl_rtt_type	*tt_vsrc;
	diskoff_t                       first_type_off;
	uint64_t			idx;
	const struct fsl_rtt_virt	*vt;

	vt = rtm->rtm_virt;

	DEBUG_VIRT_ENTER();

	DEBUG_VIRT_WRITE("BEFORE FINDING IDXS");

	/* XXX these should be invalidated when underlying changes.. need
	 * to use logging facility */
	rtm->rtm_cached_minidx = rtm->rtm_virt->vt_iter.it_min(rtm->rtm_clo);
	if (rtm->rtm_cached_minidx == ~0) {
		DEBUG_VIRT_LEAVE();
		return false;
	}
	rtm->rtm_cached_maxidx = rtm->rtm_virt->vt_iter.it_max(rtm->rtm_clo);

	DEBUG_VIRT_WRITE("MINIDX=%"PRIu64" / MAXIDX=%"PRIu64,
			rtm->rtm_cached_minidx,
			rtm->rtm_cached_maxidx);

	/* no data associated with this virt type-- don't allocate */
	if (rtm->rtm_cached_minidx > rtm->rtm_cached_maxidx) {
		DEBUG_VIRT_LEAVE();
		return false;
	}

	tt_vsrc = tt_by_num(vt->vt_iter.it_type_dst);

	uint64_t        params[tt_vsrc->tt_param_c];

	DEBUG_VIRT_WRITE("1st RANGE. %s. param_c=%d",
		tt_vsrc->tt_name,
		tt_vsrc->tt_param_c);
	first_type_off = vt->vt_iter.it_range(
		rtm->rtm_clo,
		rtm->rtm_cached_minidx,
		params);

	DEBUG_VIRT_WRITE("NEW_VCLO");
	NEW_VCLO(vsrc_clo, first_type_off, params, rtm->rtm_clo->clo_xlate);

	DEBUG_VIRT_WRITE("Get SIZE");
	rtm->rtm_cached_srcsz = tt_vsrc->tt_size(&vsrc_clo);
	FSL_ASSERT (rtm->rtm_cached_srcsz > 0);


	DEBUG_VIRT_WRITE("Looping through all source types. Verify size.");
	/* verify that srcsz is constant-- if not we need to do some other
	 * tricks */

	if (no_verify) {
		DEBUG_VIRT_LEAVE();
		return true;
	}

	/* IN FUTURE: use compiler information to judge this */
	for (	idx = rtm->rtm_cached_minidx + 1;
		idx <= rtm->rtm_cached_maxidx;
		idx++)
	{
		diskoff_t	cur_off;
		size_t		cur_sz;
		DEBUG_VIRT_WRITE("calling vt_range on idx=%d", idx);
		cur_off = vt->vt_iter.it_range(rtm->rtm_clo, idx, params);
		NEW_VCLO(cur_clo, cur_off, params, rtm->rtm_clo->clo_xlate);
		DEBUG_VIRT_WRITE("calling tt_size on idx=%d", idx);
		cur_sz = tt_vsrc->tt_size(&cur_clo);
		FSL_ASSERT (cur_sz == rtm->rtm_cached_srcsz);
	}

	/* XXX TODO: verify virtualtype fits in given source data */
	DEBUG_VIRT_LEAVE();
	return true;
}

/* verify that the type does not overflow the virtual range:
 * check size of current type against length of virt range */
static bool fsl_virt_nth_verify_bounds(
	const struct fsl_rt_mapping* rtm,
	const struct fsl_rtt_type* tt,
	uint64_t cur_off)
{
	NEW_VCLO	(new_clo, cur_off, NULL, rtm);
	typesize_t	cur_size;

	FSL_ASSERT (fsl_env->fctx_except.ex_in_unsafe_op == false);

	fsl_env->fctx_except.ex_err_unsafe_op = 0;
	fsl_env->fctx_except.ex_in_unsafe_op = true;
	cur_size = tt->tt_size(&new_clo);
	fsl_env->fctx_except.ex_in_unsafe_op = false;

	if (fsl_env->fctx_except.ex_err_unsafe_op != 0) {
		DEBUG_VIRT_WRITE("nth: bad ret on nth_verify_bounds");
		fsl_env->fctx_except.ex_err_unsafe_op = 0;
		return false;
	}

	if (cur_size + cur_off > fsl_virt_total_bits(rtm)) {
		DEBUG_VIRT_WRITE(
			"fsl_virt_get_nth: Bad: voff=%"PRIu64
			"cur_end = %"PRIu64". virt_end=%"PRIu64,
			cur_off,
			cur_size + cur_off, fsl_virt_total_bits(rtm));
		return false;
	}

	DEBUG_VIRT_WRITE("fsl_virt_get_nth: Bounds OK: voff=%"PRIu64, cur_off);

	return true;
}

/** similar to computeArrayBits, but may fail (safely). Returns a voff */
diskoff_t fsl_virt_get_nth(
	const struct fsl_rt_mapping* rtm, unsigned int target_idx)
{
	const struct fsl_rtt_type	*tt;
	unsigned int			i;
	diskoff_t			cur_off;
	typesize_t			total_bits;
	typenum_t			virt_typenum;

	FSL_ASSERT (rtm != NULL);

	if (target_idx == 0) return 0;

	DEBUG_VIRT_ENTER();

	total_bits = 0;
	cur_off = 0;
	virt_typenum = rtm->rtm_virt->vt_type_virttype;
	tt = tt_by_num(virt_typenum);
	DEBUG_VIRT_WRITE("nth of type: %s", tt->tt_name);

	for (i = 0; i < target_idx; i++) {
		typesize_t		cur_size;
		NEW_VCLO		(new_clo, cur_off, NULL, rtm);

		cur_size = tt->tt_size(&new_clo);
		DEBUG_VIRT_WRITE("nth: new_clo->voffset = %"PRIu64, new_clo.clo_offset);
		DEBUG_VIRT_WRITE("nth: fsl_total_bits= %"PRIu64, fsl_virt_total_bits(rtm));
		DEBUG_VIRT_WRITE("nth: cur_size = %"PRIu64, cur_size);
		total_bits += cur_size;
		cur_off += cur_size;

		if (total_bits >= fsl_virt_total_bits(rtm)) {
			DEBUG_VIRT_WRITE("nth %"PRIu64" >= %"PRIu64"??? Invalid!",
				total_bits,
				fsl_virt_total_bits(rtm));

			total_bits = OFFSET_INVALID;
			goto done;
		}

		if (cur_size == 0) {
			total_bits = OFFSET_EOF;
			goto done;
		}
	}

	DEBUG_VIRT_WRITE("fsl_virt_get_nth: verify bounds idx=%d voff=%"PRIu64,
		target_idx, cur_off);
	if (fsl_virt_nth_verify_bounds(rtm, tt, total_bits) == false) {
		DEBUG_VIRT_WRITE("nth: bad bounds on %"PRIu64, total_bits);
		total_bits = OFFSET_EOF;
	}

	/* reset to original */
done:
	DEBUG_VIRT_LEAVE();

	return total_bits;
}

uint64_t __toPhys(const struct fsl_rt_closure* clo, uint64_t off)
{
	if (clo->clo_xlate == NULL) return off;
	return fsl_virt_xlate(clo, off);
}

static void fsl_virt_cache_update(
	struct fsl_rt_mapping* rtm, int idx, uint64_t v)
{
	struct fsl_virtc_ent	*vc;
	vc = &rtm->rtm_cache[idx % VIRT_CACHE_ENTS];
	vc->vc_idx = idx;
	vc->vc_off = v;
}

static diskoff_t fsl_virt_cache_find(const struct fsl_rt_mapping* rtm, int idx)
{
	struct fsl_virtc_ent	*vc;
	vc = &rtm->rtm_cache[idx % VIRT_CACHE_ENTS];
	if (vc->vc_idx != idx) return ~0;
	return vc->vc_off;
}

static void fsl_virt_init_cache(struct fsl_rt_mapping* rtm)
{
	int	i;
	for (i = 0; i < VIRT_CACHE_ENTS; i++)
		rtm->rtm_cache[i].vc_idx = i+1;	/* idx%ENTS != i => invalid*/
}
