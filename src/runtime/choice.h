#ifndef CHOICE_H
#define CHOICE_H

#include "bitmap.h"

struct choice_cache {
	struct bitmap	cc_bmp;	/* set => available / unset => unavail */
	unsigned int	cc_min;
	unsigned int	cc_max;
};

#define choice_set(x,y)		bmp_set(&(x)->cc_bmp, (y), 1)
#define choice_unset(x,y)	bmp_unset(&(x)->cc_bmp, (y), 1)
#define choice_get_setidx(x,y)	bmp_get_set(&(x)->cc_bmp, (y))
#define choice_is_set(x,y)	bmp_is_set(&(x)->cc_bmp, (y))
#define choice_max(x)		(x)->cc_max
#define choice_min(x)		(x)->cc_min

struct choice_cache* choice_alloc(
	struct type_info* ti, const struct fsl_rtt_reloc* rel);
void choice_free(struct choice_cache* choice);
int choice_find_avail(
	struct choice_cache* choice,
	unsigned int offset,
	unsigned int min_count);

#endif
