#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "runtime.h"

static bool fsl_virt_load_cache(struct fsl_rt_mapping* rtm);

uint64_t fsl_virt_xlate(const struct fsl_rt_closure* clo, uint64_t bit_off)
{
	struct fsl_rt_mapping	*rtm;
	struct fsl_rt_closure	*old_clo;
	uint64_t		idx, off;
	diskoff_t		base;

	rtm = clo->clo_xlate;
	assert (rtm != NULL);

	assert (bit_off < fsl_virt_total_bits(rtm));

	idx = bit_off / rtm->rtm_cached_srcsz;
	idx += rtm->rtm_cached_minidx;	/* rebase. */

	off = bit_off % rtm->rtm_cached_srcsz;

	uint64_t params[tt_by_num(rtm->rtm_virt->vt_type_src)->tt_param_c];

	old_clo = fsl_rt_dyn_swap(rtm->rtm_dyn);
	base = rtm->rtm_virt->vt_range(rtm->rtm_clo, idx, params);
	fsl_rt_dyn_swap(old_clo);

	assert (bit_off != base+off);

	fsl_env->fctx_stat.s_xlate_call_c++;
	fsl_env->fctx_stat.s_xlate_alloc_c++;

	return base + off;
}

void fsl_virt_free(struct fsl_rt_mapping* rtm)
{
	assert (rtm != NULL);
	fsl_dyn_free(rtm->rtm_dyn);
	free(rtm);
}

struct fsl_rt_mapping*  fsl_virt_alloc(
	struct fsl_rt_closure* parent,
	struct fsl_rt_table_virt* vt)
{
	struct fsl_rt_mapping		*rtm;

	assert (parent != NULL);
	assert (vt != NULL);

	rtm = malloc(sizeof(*rtm));
	rtm->rtm_virt = vt;
	rtm->rtm_clo = parent;
	rtm->rtm_dyn = fsl_dyn_copy(fsl_env->fctx_dyn_closures);

	if (fsl_virt_load_cache(rtm) == false) {
		/* empty type */
		fsl_virt_free(rtm);
		rtm = NULL;
	}

	return rtm;
}

static bool fsl_virt_load_cache(struct fsl_rt_mapping* rtm)
{
	struct fsl_rt_table_type	*tt_vsrc;
	diskoff_t                       first_type_off;
	uint64_t			idx;
	struct fsl_rt_table_virt	*vt;

	vt = rtm->rtm_virt;

	/* XXX these should be invalidated when underlying changes.. need
	 * to use logging facility */
	FSL_LOG_START;
	rtm->rtm_cached_minidx = rtm->rtm_virt->vt_min(rtm->rtm_clo);
	printf("LOG SIZE: %d\n", fsl_io_log_ents(fsl_get_io()));
	FSL_LOG_STOP;

	rtm->rtm_cached_maxidx = rtm->rtm_virt->vt_max(rtm->rtm_clo);

	/* no data associated with this virt type-- don't allocate */
	if (rtm->rtm_cached_minidx > rtm->rtm_cached_maxidx)
		return false;

	tt_vsrc = tt_by_num(vt->vt_type_src);

	uint64_t        params[tt_vsrc->tt_param_c];
	first_type_off = vt->vt_range(
		rtm->rtm_clo,
		rtm->rtm_cached_minidx,
		params);

	NEW_VCLO(vsrc_clo, first_type_off, params, rtm->rtm_clo->clo_xlate);

	rtm->rtm_cached_srcsz = tt_vsrc->tt_size(&vsrc_clo);
	assert (rtm->rtm_cached_srcsz > 0);

	/* verify that srcsz is constant-- if not we need to do some other
	 * tricks */

	/* IN FUTURE: use compiler information to judge this */
	for (	idx = rtm->rtm_cached_minidx + 1;
		idx <= rtm->rtm_cached_maxidx;
		idx++)
	{
		diskoff_t	cur_off;
		size_t		cur_sz;
		cur_off = vt->vt_range(rtm->rtm_clo, idx, params);
		NEW_VCLO(cur_clo, cur_off, params, rtm->rtm_clo->clo_xlate);
		cur_sz = tt_vsrc->tt_size(&cur_clo);
		assert (cur_sz == rtm->rtm_cached_srcsz);
	}

	/* XXX TODO: verify virtualtype fits in given source data */

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

	total_bits = 0;
	cur_off = 0;
	virt_typenum = rtm->rtm_virt->vt_type_virttype;
	tt = tt_by_num(virt_typenum);

	/* save old dyn closure value */
	NEW_EMPTY_CLO			(old_dyn, virt_typenum);
	__getDynClosure(virt_typenum, &old_dyn);
	for (i = 0; i < target_idx; i++) {
		typesize_t		cur_size;
		NEW_CLO			(new_clo, cur_off, NULL);

		if (total_bits >= fsl_virt_total_bits(rtm)) {
			__setDyn(virt_typenum, &old_dyn);
			return OFFSET_INVALID;
		}

		__setDyn(virt_typenum, &new_clo);
		cur_size = tt->tt_size(&new_clo);
		total_bits += cur_size;
		cur_off += cur_size;
	}

	/* reset to original */
	__setDyn(virt_typenum, &old_dyn);

	return total_bits;
}
