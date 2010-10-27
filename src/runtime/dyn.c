#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "runtime.h"
#include "debug.h"

extern struct fsl_rt_ctx	*fsl_env;
extern uint64_t			fsl_num_types;

struct fsl_rt_closure* fsl_dyn_alloc(void)
{
	struct fsl_rt_closure	*dyns;
	unsigned int		i;

	if (fsl_env != NULL) {
		FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_DYNALLOC);
	}

	dyns = malloc(sizeof(struct fsl_rt_closure)*fsl_num_types);

	/* initialize all dynamic closures */
	for (i = 0; i < fsl_num_types; i++) {
		struct fsl_rt_table_type	*tt;
		struct fsl_rt_closure		*cur_clo;
		uint64_t			*param_ptr;

		cur_clo = &dyns[i];
		tt = tt_by_num(i);
		if (tt->tt_param_c == 0) {
			param_ptr = NULL;
		} else {
			unsigned int	param_byte_c;
			param_byte_c = sizeof(uint64_t) * tt->tt_param_c;
			param_ptr = malloc(param_byte_c);
			memset(param_ptr, 0xff, param_byte_c);
		}

		cur_clo->clo_offset = ~0;	/* invalid offset*/
		cur_clo->clo_params = param_ptr;
		cur_clo->clo_xlate = NULL;
	}

	return dyns;
}

void fsl_dyn_free(struct fsl_rt_closure* dyn_closures)
{
	unsigned int	i;

	for (i = 0; i < fsl_num_types; i++) {
		struct fsl_rt_closure*	clo;
		clo = &dyn_closures[i];
		if (clo->clo_params != NULL)
			free(clo->clo_params);
		fsl_virt_unref(clo);
	}
	free(dyn_closures);
}

struct fsl_rt_closure* fsl_dyn_copy(const struct fsl_rt_closure* src)
{
	struct fsl_rt_closure	*ret;
	unsigned int		i;

	assert (src != NULL);

	ret = fsl_dyn_alloc();
	for (i = 0; i < fsl_num_types; i++) {
		struct fsl_rt_closure	*cur_clo;

		cur_clo = &ret[i];
		cur_clo->clo_offset = src[i].clo_offset;
		cur_clo->clo_xlate = src[i].clo_xlate;
		if (cur_clo->clo_params != NULL)
			memcpy(	cur_clo->clo_params,
				src[i].clo_params,
				tt_by_num(i)->tt_param_c*sizeof(uint64_t));
		fsl_virt_ref(cur_clo);
	}

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_DYNCOPY);

	return ret;
}

uint64_t __getDynOffset(uint64_t type_num)
{
	const struct fsl_rt_closure	*src_clo;

	assert (type_num < fsl_env->fctx_num_types);

	src_clo = env_get_dyn_clo(type_num);
	assert (src_clo->clo_offset != ~0);

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_GETOFFSET);

	return src_clo->clo_offset;
}

void __getDynParams(uint64_t typenum, parambuf_t params_out)
{
	struct fsl_rt_table_type	*tt;

	assert (typenum < fsl_env->fctx_num_types);

	tt = tt_by_num(typenum);
	if (tt->tt_param_c == 0) return;

	memcpy(	params_out,
		env_get_dyn_clo(typenum)->clo_params,
		sizeof(uint64_t)*tt->tt_param_c);

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_GETPARAM);
}

void __getDynClosure(uint64_t typenum, struct fsl_rt_closure* clo)
{
	const struct fsl_rt_closure*	src_clo;
	unsigned int			param_c;

	assert (typenum < fsl_env->fctx_num_types);
	assert (clo != NULL);

	src_clo = env_get_dyn_clo(typenum);
	clo->clo_offset = src_clo->clo_offset;
	param_c = tt_by_num(typenum)->tt_param_c;

	assert ((param_c == 0 || clo->clo_params != NULL) && "BAD INPUT CLO BUF");
	assert ((param_c == 0 || src_clo->clo_params != NULL) && "BAD DYN SRC CLO");

	DEBUG_DYN_WRITE("copying closure params %p <- %p (xlate=%p)",
		clo->clo_params,
		src_clo->clo_params,
		clo->clo_xlate);
	memcpy(	clo->clo_params,
		src_clo->clo_params,
		sizeof(uint64_t)*tt_by_num(typenum)->tt_param_c);
	DEBUG_DYN_WRITE("successfully copied closure params");

	clo->clo_xlate = src_clo->clo_xlate;

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_GETCLOSURE);
}

void* __getDynVirt(uint64_t type_num)
{
	const struct fsl_rt_closure	*src_clo;

	assert (type_num < fsl_env->fctx_num_types);

	src_clo = env_get_dyn_clo(type_num);
	assert (src_clo->clo_offset != ~0);

	return src_clo->clo_xlate;
}

void __setDyn(uint64_t type_num, const struct fsl_rt_closure* clo)
{
	struct fsl_rt_table_type	*tt;
	struct fsl_rt_closure		*dst_clo;

	assert (type_num < fsl_env->fctx_num_types);
	tt = tt_by_num(type_num);

	dst_clo = env_get_dyn_clo(type_num);
	dst_clo->clo_offset = clo->clo_offset;
	memcpy(dst_clo->clo_params, clo->clo_params, 8*tt->tt_param_c);

	if (dst_clo->clo_xlate != clo->clo_xlate) {
		if (dst_clo->clo_xlate != NULL) fsl_virt_unref(dst_clo);
		dst_clo->clo_xlate = clo->clo_xlate;
		if (dst_clo->clo_xlate != NULL) fsl_virt_ref(dst_clo);
	}

	if (dst_clo->clo_xlate != NULL)
		assert (dst_clo->clo_xlate->rtm_clo != NULL);

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_DYNSET);
}

void fsl_dyn_dump(void)
{
	unsigned int	i;

	assert (fsl_env != NULL);
	DEBUG_WRITE("dumping dyns.");
	for (i = 0; i < fsl_env->fctx_num_types; i++) {
		DEBUG_WRITE("type %2d (%s): %"PRIu64" (%p)",
			i,
			tt_by_num(i)->tt_name,
			env_get_dyn_clo(i)->clo_offset,
			env_get_dyn_clo(i)->clo_xlate);
	}
}
