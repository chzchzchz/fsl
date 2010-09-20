#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include "runtime.h"

extern struct fsl_rt_ctx* 	fsl_env;

uint64_t fsl_virt_xlate(const struct fsl_rt_closure* clo, uint64_t bit_off)
{
	struct fsl_rt_mapping	*rtm;
	struct fsl_rt_closure	*old_clo;
	uint64_t		total_bits;
	uint64_t		idx, off;
	diskoff_t		base;

	/* sloppy implementation: improve later
	 * (read: never. research qualityyyyyy) */
	rtm = clo->clo_xlate;
	assert (rtm != NULL);

	total_bits = 1+(rtm->rtm_cached_maxidx - rtm->rtm_cached_minidx);
	total_bits *= rtm->rtm_cached_srcsz;

	assert (bit_off < total_bits);

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
	struct fsl_rt_table_type	*tt_vsrc;
	diskoff_t                       first_type_off;

	assert (parent != NULL);
	assert (vt != NULL);

	rtm = malloc(sizeof(*rtm));
	rtm->rtm_virt = vt;
	rtm->rtm_clo = parent;

	rtm->rtm_cached_minidx = rtm->rtm_virt->vt_min(rtm->rtm_clo);
	rtm->rtm_cached_maxidx = rtm->rtm_virt->vt_max(rtm->rtm_clo);

	tt_vsrc = tt_by_num(vt->vt_type_src);

	rtm->rtm_dyn = fsl_dyn_copy(fsl_env->fctx_dyn_closures);

	uint64_t        params[tt_vsrc->tt_param_c];
	first_type_off = vt->vt_range(
		rtm->rtm_clo,
		rtm->rtm_cached_minidx,
		params);

	NEW_VCLO(vsrc_clo, first_type_off, params, parent->clo_xlate);

	/* XXX not always correct-- sizes may vary among elements */
	rtm->rtm_cached_srcsz = tt_vsrc->tt_size(&vsrc_clo);
	assert (rtm->rtm_cached_srcsz > 0);

	return rtm;
}
