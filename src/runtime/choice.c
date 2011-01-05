//#define DEBUG_TOOL
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "type_info.h"
#include "choice.h"
#include "debug.h"

struct choice_cache*
choice_alloc(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	struct choice_cache*	ret;
	int			choice_min, choice_max, cur_choice;

	choice_min = rel->rel_choice.it_min(&ti_clo(ti));
	choice_max = rel->rel_choice.it_max(&ti_clo(ti));
	if (choice_max < choice_min)
		return NULL;

	DEBUG_TOOL_WRITE("Loading choice\n");

	ret = malloc(sizeof(struct choice_cache));
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

	DEBUG_TOOL_WRITE(
		"Done loading choice %d elems\n",
		choice_max - choice_min + 1);

	return ret;
}

void choice_free(struct choice_cache* choice)
{
	assert (choice != NULL);
	bmp_uninit(&choice->cc_bmp);
	free(choice);
}

int choice_find_avail(
	struct choice_cache* cc,
	unsigned int offset,
	unsigned int min_count)
{
	int	bmp_base_off, bmp_cur_off;
	int	max_bits;

	assert (offset >= cc->cc_min && "OFFSET LESS THAN CC_MIN");
	assert (offset <= cc->cc_max && "OFFSET MORE THAN CC_MAX");
	bmp_base_off = offset - cc->cc_min;

	bmp_cur_off = bmp_base_off;
	max_bits = bmp_num_bits(&cc->cc_bmp);
	do {
		int	found_bits;
		if (bmp_is_set(&cc->cc_bmp, bmp_cur_off)) {
			found_bits = bmp_count_contig_set(
				&cc->cc_bmp, bmp_cur_off);
			if (found_bits >= min_count) {
				return bmp_cur_off + cc->cc_min;
			}
		} else {
			found_bits = bmp_count_contig_avail(
				&cc->cc_bmp, bmp_cur_off);
		}
		bmp_cur_off += found_bits;
	} while(bmp_cur_off < max_bits);

	return -1;
}
