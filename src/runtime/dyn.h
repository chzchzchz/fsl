#ifndef DYN_H
#define DYN_H

#define DYN_INVALID_TYPE	(~((uint64_t)0))
#define env_get_dyn_clo(x)	(&(fsl_env->fctx_dyn_closures[(x)]))

#define FSL_DYN_SAVE(x)		struct fsl_rt_closure*	x;			\
				x = fsl_dyn_copy(fsl_env->fctx_dyn_closures)
#define FSL_DYN_LOAD(x)		fsl_dyn_free(					\
					fsl_rt_dyn_swap(			\
						fsl_dyn_copy(x)))

#define FSL_DYN_RESTORE(x)	do {					\
									\
					fsl_dyn_free(fsl_rt_dyn_swap(x));	\
					x = NULL;			\
				} while (0)

struct fsl_rt_closure* fsl_rt_dyn_swap(struct fsl_rt_closure* dyns);
void fsl_dyn_dump(void);
struct fsl_rt_closure* fsl_dyn_alloc(void);
void fsl_dyn_free(struct fsl_rt_closure*);
struct fsl_rt_closure* fsl_dyn_copy(const struct fsl_rt_closure* src);

#endif
