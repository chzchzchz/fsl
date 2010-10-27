#ifndef TYPE_INFO_H
#define TYPE_INFO_H

#include "runtime.h"

/* fully describes a type instance */
struct type_desc
{
	typenum_t			td_typenum;
	struct fsl_rt_closure		td_clo;
};

#define TI_INVALID_IDXVAL	(~0)

/* extra run-time info */
struct type_info
{
	struct type_desc		ti_td;
	const struct type_info		*ti_prev;

	const char			*ti_print_name;
	uint64_t			ti_print_idxval;

	/* how we descended from parent */
	const struct fsl_rt_table_field		*ti_field;
	const struct fsl_rt_table_virt		*ti_virt;
	const struct fsl_rt_table_pointsto	*ti_points;

	unsigned int		ti_depth;
};


#define td_origin()	{.td_typenum = fsl_rt_origin_typenum, 	\
			.td_clo = { 				\
				.clo_offset = 0, 		\
				.clo_params = NULL, 		\
				.clo_xlate = NULL}}


#define ti_to_thunk(x)	td_to_thunk(&((x)->ti_td))
#define ti_to_td(x)	(&((x)->ti_td))
#define TI_INTO_CLO(x)	TD_INTO_CLO(&(x)->ti_td)
#define TI_INTO_CLO_DECL(x,y) TD_INTO_CLO_DECL(x, &((y)->ti_td))

#define TD_INTO_CLO_DECL(x,y)					\
			struct fsl_rt_closure x;		\
			x.clo_offset = (y)->td_clo.clo_offset;	\
			x.clo_params = (y)->td_clo.clo_params;	\
			x.clo_xlate = (y)->td_clo.clo_xlate;

#define TD_INTO_CLO(x)	const struct fsl_rt_closure *clo = &((x)->td_clo);

#define td_explode(x)	((x)->td_typenum), (&((x)->td_clo))
#define td_by_ti(x)	(&((x)->ti_td))
#define tt_by_ti(x)	tt_by_num(ti_typenum(x))
#define td_offset(x)	(x)->td_clo.clo_offset
#define td_params(x)	(x)->td_clo.clo_params
#define td_xlate(x)	(x)->td_clo.clo_xlate

#define td_vinit(x,t,y,z,v)	do {				\
				(x)->td_typenum = t;		\
				(x)->td_clo.clo_offset = y;	\
				(x)->td_clo.clo_params = z;	\
				(x)->td_clo.clo_xlate = v;	\
			} while (0)
#define td_init(x,t,y,z)	td_vinit(x,t,y,z,NULL)

#define td_typenum(x)	(x)->td_typenum
#define ti_typenum(x)	td_typenum(ti_to_td(x))
#define ti_offset(x)	td_offset(ti_to_td(x))
typesize_t ti_size(const struct type_info* ti);
poff_t ti_phys_offset(const struct type_info* ti);
#define ti_params(x)	td_params(ti_to_td(x))
#define ti_xlate(x)	td_xlate(ti_to_td(x))
#define ti_clo(x)	(ti_to_td(x))->td_clo

#define ti_type_name(x)	((ti_typenum(x) == TYPENUM_INVALID) ?	\
				"NO_TYPE" : 		\
				tt_by_ti(x)->tt_name)

#define typeinfo_set_depth(x,y)	do { (x)->ti_depth = (y); } while (0)
#define ti_depth(x)	(x)->ti_depth

void typeinfo_free(struct type_info* ti);
struct type_info* typeinfo_alloc_by_field(
	const struct type_desc* ti_td,
	const struct fsl_rt_table_field	*ti_field,
	const struct type_info*	ti_prev);
struct type_info* typeinfo_alloc_pointsto(
	const struct type_desc			*ti_td,
	const struct fsl_rt_table_pointsto	*ti_pointsto,
	unsigned int				ti_pointsto_elem,
	const struct type_info			*ti_prev);

#define typeinfo_alloc_virt(v,t)	typeinfo_alloc_virt_idx(v,t,0,NULL)

#define TI_ERR_OK		0
#define TI_ERR_BADVIRT		-1
#define TI_ERR_BADIDX		-2
#define TI_ERR_BADALLOC		-3
#define TI_ERR_BADVERIFY	-4

struct type_info* typeinfo_alloc_virt_idx(
	struct fsl_rt_table_virt* virt,
	struct type_info*	ti_prev,
	unsigned int		idx_no,
	int			*err_code);
struct type_info* typeinfo_virt_next(struct type_info* ti, int* err_code);


void typeinfo_set_dyn(const struct type_info* ti);

#include "type_print.h"

#endif
