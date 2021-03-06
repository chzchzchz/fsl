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
	struct type_info		*ti_prev;

	unsigned int			ti_ref_c;

	const char			*ti_print_name;
	uint64_t			ti_print_idxval;

	/* how we descended from parent */
	const struct fsl_rtt_field	*ti_field;
	const struct fsl_rtt_virt	*ti_virt;
	const struct fsl_rtt_pointsto	*ti_points;
	const struct fsl_rt_iter	*ti_iter;

	unsigned int		ti_flag;
	unsigned int		ti_depth;
};

#define TI_FL_FROMNAME	1
#define ti_from_name(x)	((x)->ti_flag & TI_FL_FROMNAME)
#define ti_is_field(x)	((x)->ti_field != NULL)
#define ti_is_virt(x)	((x)->ti_virt != NULL)
#define ti_is_points(x) ((x)->ti_points != NULL)
#define ti_is_iter(x)	((x)->ti_iter != NULL)

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
typesize_t ti_field_size(
	const struct type_info* ti_parent, const struct fsl_rtt_field* tf,
	uint64_t* num_elems);
typesize_t ti_field_off(
	const struct type_info* ti, const struct fsl_rtt_field* tf,
	unsigned int idx);
poff_t ti_phys_offset(const struct type_info* ti);
struct type_info* typeinfo_ref(struct type_info* ti);
#define ti_params(x)	td_params(ti_to_td(x))
#define ti_xlate(x)	td_xlate(ti_to_td(x))
#define ti_clo(x)	(ti_to_td(x))->td_clo

#define ti_type_name(x)	((ti_typenum(x) == TYPENUM_INVALID) ?	\
				"NO_TYPE" : 		\
				tt_by_ti(x)->tt_name)

#define typeinfo_set_depth(x,y)	do { (x)->ti_depth = (y); } while (0)
#define ti_depth(x)	(x)->ti_depth

struct type_info* typeinfo_alloc_origin(void);

void typeinfo_free(struct type_info* ti);
struct type_info* typeinfo_alloc_by_field(
	const struct type_desc		*ti_td,
	const struct fsl_rtt_field	*ti_field,
	struct type_info*		ti_prev);
struct type_info* typeinfo_alloc_pointsto(
	const struct type_desc		*ti_td,
	const struct fsl_rtt_pointsto	*ti_pointsto,
	unsigned int			ti_pointsto_elem,
	struct type_info*		ti_prev);
struct type_info* typeinfo_alloc_iter(
	const struct type_desc		*ti_td,
	const struct fsl_rt_iter	*ti_iter,
	unsigned int			ti_iter_elem,
	struct type_info*		ti_prev);

struct type_info* typeinfo_follow_into_name(
	struct type_info*	ti,
	const char*		fieldname,
	const char*		name);
#define typeinfo_alloc_virt(v,t)	typeinfo_alloc_virt_idx(v,t,0,NULL)
#define typeinfo_follow_virt(ti, vt, idx, err) typeinfo_alloc_virt_idx(vt, ti, idx, err)

#define TI_ERR_OK		0
#define TI_ERR_BADVIRT		-1	/* could not allocate virt */
#define TI_ERR_BADIDX		-2	/* offset into virt invalid */
#define TI_ERR_BADALLOC		-3	/* could not allocate typeinfo */
#define TI_ERR_BADVERIFY	-4	/* loop and/or assertions failed */
#define TI_ERR_EOF		-5	/* out of bounds access */
#define ti_err_continue(x)	(((x) == 0 || (x) == TI_ERR_BADVIRT))

struct type_info* typeinfo_alloc_virt_idx(
	const struct fsl_rtt_virt* virt,
	struct type_info*	ti_prev,
	unsigned int		idx_no,
	int			*err_code);
struct type_info* typeinfo_virt_next(struct type_info* ti, int* err_code);


struct type_info* typeinfo_follow_field_off_idx(
	struct type_info*		ti_parent,
	const struct fsl_rtt_field* ti_field,
	diskoff_t			offset,
	uint64_t			idx);

#define typeinfo_follow_field_idx(tip,tif,idx)				\
	 typeinfo_follow_field_off_idx(					\
		tip, tif, ti_field_off(tip, tif, idx), idx)

#define typeinfo_follow_field_off(tip,tif,off)			\
		typeinfo_follow_field_off_idx(tip,tif,off,0)
struct type_info* typeinfo_follow_field(
	struct type_info* tip, const struct fsl_rtt_field* tif);

struct type_info* typeinfo_follow_pointsto(
	struct type_info*		ti_parent,
	const struct fsl_rtt_pointsto*	ti_pt,
	uint64_t			idx);
struct type_info* typeinfo_follow_iter(
	struct type_info*		ti_parent,
	const struct fsl_rt_iter*	ti_iter,
	uint64_t			idx);

#define typeinfo_lookup_follow(x,y)	typeinfo_lookup_follow_idx_all(x,y,0,true)
#define typeinfo_lookup_follow_direct(x,y) typeinfo_lookup_follow_idx_all(x,y,0,false)
#define typeinfo_lookup_follow_idx(x,y,z) typeinfo_lookup_follow_idx_all(x,y,z,false)
struct type_info* typeinfo_lookup_follow_idx_all(
	struct type_info*	ti_parent,
	const char*		fieldname,
	uint64_t		idx,
	bool			accept_names);
struct type_info* typeinfo_follow_name_tf(
	struct type_info		*ti,
	const struct fsl_rtt_field	*tf,
	const char			*name);
struct type_info* typeinfo_follow_name_pt(
	struct type_info		*ti,
	const struct fsl_rtt_pointsto	*pt,
	const char			*fieldname);
struct type_info* typeinfo_follow_name_vt(
	struct type_info		*ti,
	const struct fsl_rtt_virt	*vt,
	const char			*fieldname);

void typeinfo_phys_copy(struct type_info* dst, struct type_info* src);
unsigned int typeinfo_to_buf(struct type_info* src, void* buf, unsigned int bytes);
struct type_info* typeinfo_reindex(
	const struct type_info* dst, unsigned int idx);
char* typeinfo_getname(struct type_info* ti, char* buf, unsigned int len);

#include "type_print.h"

#endif
