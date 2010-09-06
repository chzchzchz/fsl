#ifndef TYPE_INFO_H
#define TYPE_INFO_H

#include "runtime.h"

struct type_desc
{
	typenum_t	td_typenum;
	diskoff_t	td_diskoff;
	parambuf_t	td_params;
};

struct type_info
{
	struct type_desc	ti_td;
	bool			ti_is_virt;
	bool			ti_pointsto;
	union {
		unsigned int		ti_fieldidx;
		unsigned int		ti_virttype_c;	/* num repetitions */
		struct {
			unsigned int	ti_pointstoidx;
			unsigned int	ti_pointsto_elem; /* for arrays */
		};
	};

	unsigned int		ti_depth;
	const struct type_info	*ti_prev;
};

#define ti_typenum(x)	(x)->ti_td.td_typenum
#define ti_to_thunk(x)	td_to_thunk(&((x)->ti_td))
#define td_to_thunk(x)	((x)->td_diskoff), ((x)->td_params)
#define td_explode(x)	((x)->td_typenum), ((x)->td_diskoff), ((x)->td_params)
#define td_by_ti(x)	(&((x)->ti_td))
#define tt_by_ti(x)	tt_by_num(ti_typenum(x))

#define typeinfo_set_depth(x,y)	do { (x)->ti_depth = (y); } while (0)
#define typeinfo_get_depth(x)	(x)->ti_depth

void typeinfo_print(const struct type_info* ti);
void typeinfo_print_fields(const struct type_info* ti);
void typeinfo_free(struct type_info* ti);
struct type_info* typeinfo_alloc(
	const struct type_desc* ti_td,
	unsigned int		ti_fieldidx,
	const struct type_info*	ti_prev);
struct type_info* typeinfo_alloc_pointsto(
	const struct type_desc* ti_td,
	unsigned int		ti_pointsto_idx,
	unsigned int		ti_pointsto_elem,
	const struct type_info*	ti_prev);

struct type_info* typeinfo_alloc_virt(
	struct fsl_rt_table_virt* virt,
	const struct type_info*	ti_prev);


void typeinfo_print_name(void);
void typeinfo_print_path(const struct type_info* cur);
void typeinfo_print_pointsto(const struct type_info* cur);
void typeinfo_dump_data(const struct type_info* ti);
void typeinfo_set_dyn(const struct type_info* ti);

#endif
