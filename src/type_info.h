#ifndef TYPE_INFO_H
#define TYPE_INFO_H

#include "runtime.h"

struct type_info
{
	typenum_t		ti_typenum;
	diskoff_t		ti_diskoff;

	bool			ti_pointsto;
	union {
		unsigned int		ti_fieldidx;
		struct {
			unsigned int	ti_pointstoidx;
			unsigned int	ti_pointsto_elem; /* for arrays */
		};
	};

	unsigned int		ti_depth;
	struct type_info	*ti_prev;
};

#define tt_by_ti(x)	tt_by_num((x)->ti_typenum)

void typeinfo_print(const struct type_info* ti);
void typeinfo_print_fields(const struct type_info* ti);
void typeinfo_free(struct type_info* ti);
struct type_info* typeinfo_alloc(
	typenum_t		ti_typenum,
	diskoff_t		ti_diskoff,
	unsigned int		ti_fieldidx,
	struct type_info*	ti_prev);
struct type_info* typeinfo_alloc_pointsto(
	typenum_t		ti_typenum,
	diskoff_t		ti_diskoff,
	unsigned int		ti_pointsto_idx,
	unsigned int		ti_pointsto_elem,
	struct type_info*	ti_prev);

void typeinfo_print_name(void);
void typeinfo_print_path(const struct type_info* cur);
void typeinfo_print_pointsto(const struct type_info* cur);
void typeinfo_dump_data(const struct type_info* ti);
void typeinfo_set_dyn(const struct type_info* ti);

#endif
