//#define DEBUG_VIRT
#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "runtime.h"
#include "virt.h"
#include "debug.h"

static bool fsl_virt_load_cache(struct fsl_rt_mapping* rtm, bool no_verify);

uint64_t fsl_virt_xlate(const struct fsl_rt_closure* clo, uint64_t bit_off)
{
	struct fsl_rt_mapping	*rtm;
	struct fsl_rt_closure	*old_clo;
	uint64_t		idx, off;
	diskoff_t		base;

	DEBUG_VIRT_ENTER();

	DEBUG_VIRT_WRITE("xlating!!!");

	assert (clo != NULL);

	rtm = clo->clo_xlate;
	DEBUG_VIRT_WRITE("rtmptr: %p", rtm);
	assert (rtm != NULL && "No xlate.");
	assert (rtm->rtm_clo != NULL && "xlate not fully initialized?");

	assert (bit_off < fsl_virt_total_bits(rtm));
	DEBUG_VIRT_WRITE("rtm->rtm_cached_min = %"PRIu64, rtm->rtm_cached_minidx);
	DEBUG_VIRT_WRITE("rtm->rtm_cached_max = %"PRIu64, rtm->rtm_cached_maxidx);

	idx = bit_off / rtm->rtm_cached_srcsz;
	idx += rtm->rtm_cached_minidx;	/* base. */

	off = bit_off % rtm->rtm_cached_srcsz;

	uint64_t params[tt_by_num(rtm->rtm_virt->vt_type_src)->tt_param_c];

	/* use dyn vars set on allocation of xlate */
	old_clo = fsl_rt_dyn_swap(rtm->rtm_dyn);
	DEBUG_VIRT_WRITE("&rtm->rtm_clo = %p", rtm->rtm_clo);
	DEBUG_VIRT_WRITE("rtm_clo->clo_offset: %"PRIu64" bits (%"PRIu64" bytes)",
		rtm->rtm_clo->clo_offset,
		rtm->rtm_clo->clo_offset / 8);
	base = rtm->rtm_virt->vt_range(rtm->rtm_clo, idx, params);
	DEBUG_VIRT_WRITE("BASE FOUND: %"PRIu64, base);
	DEBUG_VIRT_WRITE("WANTED BITOFF: %"PRIu64, bit_off);

	/* swap back to old dyn vars */
	fsl_rt_dyn_swap(old_clo);

	assert (bit_off != base+off && "Identity xlate. Probably wrong.");
	assert ((base+off) != 0 && "xlated addr == origin? Probably wrong.");

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_XLATE_CALL);

	DEBUG_VIRT_LEAVE();

	return base + off;
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
	assert (rtm != NULL);
	assert (rtm->rtm_ref_c == 1);
	DEBUG_VIRT_WRITE("Freeing: rtm=%p", rtm);
	fsl_dyn_free(rtm->rtm_dyn);
	fsl_virt_unref_all(rtm->rtm_clo);
	free(rtm);
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
	struct fsl_rt_table_virt* vt)
{
	struct fsl_rt_mapping		*rtm;

	assert (parent != NULL);
	assert (vt != NULL);

	DEBUG_VIRT_ENTER();
	DEBUG_VIRT_WRITE("COPYING FOR VIRT_ALLOC");

	rtm = malloc(sizeof(*rtm));
	rtm->rtm_virt = vt;
	rtm->rtm_clo = parent;
	fsl_virt_ref_all(parent);
	rtm->rtm_dyn = fsl_dyn_copy(fsl_env->fctx_dyn_closures);
	rtm->rtm_ref_c = 1;

	DEBUG_VIRT_WRITE("DYNALLOC: rtm=%p. rtm->rtm_clo=%p",
		rtm, rtm->rtm_clo);

	DEBUG_VIRT_WRITE("DYN COPY DONE");

	/* XXX for the time being, assume all source types are same size */
	if (fsl_virt_load_cache(rtm, true) == false) {
		/* empty type */
		fsl_virt_free(rtm);
		rtm = NULL;
	}

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_XLATE_ALLOC);

	DEBUG_VIRT_WRITE("fresh new RTM %p", rtm);
	DEBUG_VIRT_LEAVE();
	return rtm;
}

static bool fsl_virt_load_cache(struct fsl_rt_mapping* rtm, bool no_verify)
{
	struct fsl_rt_table_type	*tt_vsrc;
	diskoff_t                       first_type_off;
	uint64_t			idx;
	struct fsl_rt_table_virt	*vt;

	vt = rtm->rtm_virt;

	DEBUG_VIRT_ENTER();

	DEBUG_VIRT_WRITE("BEFORE FINDING IDXS");

	/* XXX these should be invalidated when underlying changes.. need
	 * to use logging facility */
	rtm->rtm_cached_minidx = rtm->rtm_virt->vt_min(rtm->rtm_clo);
	rtm->rtm_cached_maxidx = rtm->rtm_virt->vt_max(rtm->rtm_clo);

	DEBUG_VIRT_WRITE("MINIDX=%"PRIu64" / MAXIDX=%"PRIu64,
			rtm->rtm_cached_minidx,
			rtm->rtm_cached_maxidx);

	/* no data associated with this virt type-- don't allocate */
	if (rtm->rtm_cached_minidx > rtm->rtm_cached_maxidx) {
		DEBUG_VIRT_LEAVE();
		return false;
	}

	tt_vsrc = tt_by_num(vt->vt_type_src);

	uint64_t        params[tt_vsrc->tt_param_c];

	DEBUG_VIRT_WRITE("1st RANGE. %s. param_c=%d",
		tt_vsrc->tt_name,
		tt_vsrc->tt_param_c);
	first_type_off = vt->vt_range(
		rtm->rtm_clo,
		rtm->rtm_cached_minidx,
		params);

	DEBUG_VIRT_WRITE("NEW_VCLO");
	NEW_VCLO(vsrc_clo, first_type_off, params, rtm->rtm_clo->clo_xlate);

	DEBUG_VIRT_WRITE("Get SIZE");
	rtm->rtm_cached_srcsz = tt_vsrc->tt_size(&vsrc_clo);
	assert (rtm->rtm_cached_srcsz > 0);


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
		cur_off = vt->vt_range(rtm->rtm_clo, idx, params);
		NEW_VCLO(cur_clo, cur_off, params, rtm->rtm_clo->clo_xlate);
		DEBUG_VIRT_WRITE("calling tt_size on idx=%d", idx);
		cur_sz = tt_vsrc->tt_size(&cur_clo);
		assert (cur_sz == rtm->rtm_cached_srcsz);
	}

	/* XXX TODO: verify virtualtype fits in given source data */
	DEBUG_VIRT_LEAVE();
	return true;
}

/** similar to computeArrayBits, but may fail (safely) */
diskoff_t fsl_virt_get_nth(
	const struct fsl_rt_mapping* rtm, unsigned int target_idx)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;
	diskoff_t			cur_off;
	typesize_t			total_bits;
	typenum_t			virt_typenum;

	assert (rtm != NULL);

	if (target_idx == 0) return 0;

	DEBUG_VIRT_ENTER();

	total_bits = 0;
	cur_off = 0;
	virt_typenum = rtm->rtm_virt->vt_type_virttype;
	tt = tt_by_num(virt_typenum);
	DEBUG_VIRT_WRITE("nth of type: %s", tt->tt_name);

	/* save old dyn closure value */
	NEW_EMPTY_CLO			(old_dyn, virt_typenum);
	__getDynClosure(virt_typenum, &old_dyn);
	fsl_virt_ref(&old_dyn);

	for (i = 0; i < target_idx; i++) {
		typesize_t		cur_size;
		NEW_VCLO		(new_clo, cur_off, NULL, rtm);

		__setDyn(virt_typenum, &new_clo);

		cur_size = tt->tt_size(&new_clo);
		DEBUG_VIRT_WRITE("new_clo->voffset = %"PRIu64, new_clo.clo_offset);
		DEBUG_VIRT_WRITE("fsl_total_bits= %"PRIu64, fsl_virt_total_bits(rtm));
		DEBUG_VIRT_WRITE("cur_size = %"PRIu64, cur_size);
		total_bits += cur_size;
		cur_off += cur_size;

		DEBUG_VIRT_WRITE("%"PRIu64" >= %"PRIu64"??? Invalid!",
			total_bits,
			fsl_virt_total_bits(rtm));
		if (total_bits >= fsl_virt_total_bits(rtm)) {
			__setDyn(virt_typenum, &old_dyn);
			fsl_virt_unref(&old_dyn);
			DEBUG_VIRT_LEAVE();
			return OFFSET_INVALID;
		}
	}

	/* reset to original */
	__setDyn(virt_typenum, &old_dyn);
	fsl_virt_unref(&old_dyn);


	DEBUG_VIRT_LEAVE();

	return total_bits;
}
