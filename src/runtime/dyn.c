#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "runtime.h"

#define DYN_INVALID_TYPE	(~((uint64_t)0))
#define env_get_dyn_clo(x)	(&(fsl_env->fctx_dyn_closures[(x)]))
extern struct fsl_rt_ctx	*fsl_env;
extern uint64_t			fsl_num_types;

struct fsl_rt_closure* fsl_dyn_alloc(void)
{
	struct fsl_rt_closure	*dyns;
	unsigned int		i;

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
		ret[i].clo_offset = src[i].clo_offset;
		ret[i].clo_xlate = src[i].clo_xlate;
		if (ret[i].clo_params != NULL)
			memcpy(ret[i].clo_params, src[i].clo_params,
				tt_by_num(i)->tt_param_c*sizeof(uint64_t));
	}

	return ret;
}

uint64_t __getDynOffset(uint64_t type_num)
{
	const struct fsl_rt_closure	*src_clo;

	assert (type_num < fsl_env->fctx_num_types);

	src_clo = env_get_dyn_clo(type_num);
	assert (src_clo->clo_offset != ~0);

	fsl_env->fctx_stat.s_get_offset_c++;

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

	fsl_env->fctx_stat.s_get_param_c++;
}

void __getDynClosure(uint64_t typenum, struct fsl_rt_closure* clo)
{
	const struct fsl_rt_closure*	src_clo;

	assert (typenum < fsl_env->fctx_num_types);
	assert (clo != NULL);

	src_clo = env_get_dyn_clo(typenum);
	clo->clo_offset = src_clo->clo_offset;
	memcpy(	clo->clo_params,
		src_clo->clo_params,
		sizeof(uint64_t)*tt_by_num(typenum)->tt_param_c);
	clo->clo_xlate = src_clo->clo_xlate;

	fsl_env->fctx_stat.s_get_closure_c++;
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
	dst_clo->clo_xlate = clo->clo_xlate;

	fsl_env->fctx_stat.s_dyn_set_c++;
}

void fsl_dyn_dump(void)
{
	unsigned int	i;

	assert (fsl_env != NULL);
	for (i = 0; i < fsl_env->fctx_num_types; i++) {
		printf("type %2d (%s): %"PRIu64"\n",
			i,
			tt_by_num(i)->tt_name,
			env_get_dyn_clo(i)->clo_offset);
	}
}
