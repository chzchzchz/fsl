//#define DEBUG_CHOICE
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include "alloc.h"
#include "type_info.h"
#include "choice.h"
#include "debug.h"

struct choice_cache*
choice_alloc(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	struct choice_cache*	ret;
	uint64_t		choice_min, choice_max, cur_choice;

	choice_min = rel->rel_choice.it_min(&ti_clo(ti));
	choice_max = rel->rel_choice.it_max(&ti_clo(ti));
	if (choice_max < choice_min) return NULL;

	DEBUG_CHOICE_WRITE("Loading choice\n");

	ret = fsl_alloc(sizeof(struct choice_cache));
	ret->cc_min = choice_min;
	ret->cc_max = choice_max;

	bmp_init(&ret->cc_bmp, (ret->cc_max - ret->cc_min)+1);
	bmp_clear(&ret->cc_bmp);	/* mark all as allocated */

	/* test all choices, set bitmap accordingly. */
	for (cur_choice = choice_min; cur_choice <= choice_max; cur_choice++) {
		bool	c_ok;
		c_ok = rel->rel_ccond(&ti_clo(ti), cur_choice);
		if (c_ok) choice_mark_free(ret, cur_choice);
	}

	DEBUG_CHOICE_WRITE("Done loading choice %d elems",
		choice_max - choice_min + 1);

	return ret;
}

void choice_free(struct choice_cache* choice)
{
	FSL_ASSERT(choice != NULL);
	bmp_uninit(&choice->cc_bmp);
	fsl_free(choice);
}

int choice_find_avail(
	struct choice_cache* cc,
	unsigned int offset,
	unsigned int min_count)
{
	int	bmp_base_off, bmp_cur_off;
	int	max_bits;

	FSL_ASSERT(offset >= cc->cc_min && "OFFSET LESS THAN CC_MIN");
	FSL_ASSERT(offset <= cc->cc_max && "OFFSET MORE THAN CC_MAX");
	bmp_base_off = offset - cc->cc_min;

	bmp_cur_off = bmp_base_off;
	max_bits = bmp_num_bits(&cc->cc_bmp);
	do {
		int	found_bits;
		if (bmp_is_set(&cc->cc_bmp, bmp_cur_off)) {
			found_bits = bmp_count_contig_set(
				&cc->cc_bmp, bmp_cur_off);
			if (found_bits >= min_count)
				return bmp_cur_off + cc->cc_min;
		} else {
			found_bits = bmp_count_contig_avail(
				&cc->cc_bmp, bmp_cur_off);
		}
		bmp_cur_off += found_bits;
	} while(bmp_cur_off < max_bits);

	return -1;
}

void choice_dump(struct choice_cache* cc)
{
	uint64_t	cur, ext_start;
	bool		was_last_free;

	ext_start = cc->cc_min;
	was_last_free = choice_is_free(cc, ext_start);
	for (cur = ext_start+1; cur <= cc->cc_max; cur++) {
		bool	is_cur_free;
		is_cur_free = choice_is_free(cc, cur);
		if (is_cur_free != was_last_free) {
			if (was_last_free) DEBUG_WRITE("Free: ");
			else DEBUG_WRITE("Used: ");
			DEBUG_WRITE("[%"PRIu64"--%"PRIu64"]\n", ext_start, cur-1);
			was_last_free = is_cur_free;
			ext_start = cur;
		}
	}

	if (was_last_free) DEBUG_WRITE("Free: ");
	else DEBUG_WRITE("Used: ");
	DEBUG_WRITE("[%"PRIu64"--%"PRIu64"]\n", ext_start, cur-1);
}

void choice_refresh(struct choice_cache* choice, uint64_t lo, uint64_t hi)
{
	FSL_ASSERT(0 == 1 && "STUB: TODO");
}
