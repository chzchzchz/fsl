//#define DEBUG_TOOL
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include "runtime.h"
#include "debug.h"
#include "type_info.h"
#include "bitmap.h"
#include "scan.h"
#include "log.h"

struct img
{
	unsigned int	x;
	unsigned int	y;
	uint8_t		*bmp;
	uint64_t	disk_bytes;
	uint8_t		*bmp_disk;
};

struct relocscan_info
{
	unsigned int	rel_c;
};

struct choice_cache {
	struct bitmap	cc_bmp;
	unsigned int	cc_min;
	unsigned int	cc_max;
};

#define bytes_per_px(a)	((a)->disk_bytes/((a)->x*(a)->y))

static int byte_to_pixval(struct img* im, diskoff_t in_byte)
{
	return im->bmp[in_byte/bytes_per_px(im)];
}

static uint64_t byte_to_imgidx(struct img* im, diskoff_t in_byte)
{
	return in_byte/bytes_per_px(im);
}

#define ccache_set(x)		bmp_set(&ccache->cc_bmp, (x), 1)
#define ccache_unset(x)		bmp_unset(&ccache->cc_bmp, (x), 1)
#define ccache_get_setidx(x)	bmp_get_set(&ccache->cc_bmp, (x))
#define ccache_is_set(x)	bmp_is_set(&ccache->cc_bmp, (x))

static struct choice_cache	*ccache = NULL;
static struct img		*reloc_img;
static uint64_t			ccache_cursor;
static uint64_t			reloc_cursor;

/* advance cursor to next allocated space */
void reloc_img_advance_unchecked(void)
{
	int	i;
	for (i = reloc_cursor+1; i < reloc_img->x*reloc_img->y; i++) {
		if (reloc_img->bmp[i]) {
			reloc_cursor = i;
			return;
		}
	}
	reloc_cursor = ~0;
}

void reloc_img_advance(void)
{
	if (reloc_cursor == ~0) return;
	reloc_img_advance_unchecked();
}

/* XXX we are so cheating right now */
void ccache_load(struct type_info* ti, const struct fsl_rt_table_reloc* rel)
{
	int choice_min, choice_max, cur_choice;

	choice_min = rel->rel_choice.it_min(&ti_clo(ti));
	choice_max = rel->rel_choice.it_max(&ti_clo(ti));

	ccache = malloc(sizeof(struct choice_cache));
	ccache->cc_min = choice_min;
	ccache->cc_max = choice_max;
	assert (ccache->cc_max >= ccache->cc_min);
	bmp_init(&ccache->cc_bmp, (ccache->cc_max - ccache->cc_min)+1);

	/* test all choices, set bitmap accordingly */
	for (cur_choice = choice_min; cur_choice < choice_max; cur_choice++) {
		bool	c_ok;
		c_ok = rel->rel_ccond(&ti_clo(ti), cur_choice);
		if (c_ok) ccache_set(cur_choice - choice_min);
	}

	ccache_cursor = ccache->cc_min;
}

/* finds the next choice that falls on reloc_cursor
 * that is not before the ccache_cursor
 * We need to say that we need choices to hold to some invariant
 * for this to work
 */
uint64_t choice_find(
	struct type_info* ti,
	const struct fsl_rt_table_reloc* rel)
{
	unsigned int	cur_choice;
	unsigned int	param_c;

	param_c = tt_by_num(rel->rel_choice.it_type_dst)->tt_param_c;
	for (	cur_choice = ccache_cursor;
		cur_choice <= ccache->cc_max;
		cur_choice++)
	{
		uint64_t	buf[param_c];
		diskoff_t	off_bits;
		uint64_t	imgidx;

		/* check cache first */
		if (!ccache_is_set(cur_choice-ccache->cc_min)) {
			/* not OK to use */
			continue;
		}

		off_bits = rel->rel_choice.it_range(&ti_clo(ti), cur_choice, buf);
		imgidx = byte_to_imgidx(reloc_img, off_bits / 8);
		if (imgidx == reloc_cursor) {
			/* success! */
			ccache_unset(ccache_cursor);
			reloc_img_advance();
			if (reloc_cursor == ~0) {
				/* nothing left to do */
				ccache_cursor = ccache->cc_max + 1;
			} else
				ccache_cursor = cur_choice + 1;
			return cur_choice;
		} else if (imgidx > reloc_cursor) {
			/* overshot.. can't alloc */
			printf("OOPS! Can't alloc at imgidx: %"PRIu64"\n",
				reloc_cursor);

			/* move on to next one */
			reloc_img_advance();
			if (reloc_cursor == ~0) {
				/* nothing left to do */
				ccache_cursor = ccache->cc_max + 1;
				return ~0;
			}
			cur_choice--;
		}
	}

	printf("RAN OUT OF CHOICES??\n");
	return ~0;
}

/* compare location of selected with reloc locations */
/* if reloc location needs to be set, try to swap in */
/* if rel_sel location is on empty reloc location, try to swap out */
void swap_rel_sel(
	struct type_info* ti, const struct fsl_rt_table_reloc* rel,
	struct type_info* rel_sel_ti, unsigned int sel_v)
{
	diskoff_t		sel_off_bit;
	uint64_t		replace_choice_idx;
	struct type_info	*replace_choice_ti;
	int			pxval;
	struct fsl_rt_wlog	wlog_replace;

	sel_off_bit = ti_offset(rel_sel_ti);
	pxval = byte_to_pixval(reloc_img, sel_off_bit/8);

	/* already in an OK place, don't do anything
	 * (in future, may want to steal if we have overlaps) */
	if (pxval != 0) return;

	/* need to move this to an unused location that
	 * is marked in the image */
	printf("Need to move %"PRIu64"\n", sel_off_bit/8);
	replace_choice_idx = choice_find(ti, rel);
	if (replace_choice_idx == ~0) {
		printf("No more moves left. Oh well.\n");
		return;
	}

	printf(" GOT CHOICE %"PRIu64"\n", replace_choice_idx);
	replace_choice_ti = typeinfo_follow_iter(
		ti, &rel->rel_choice, replace_choice_idx);
	assert (replace_choice_ti != NULL);

	/* relocation procedure:
	 * 1. allocate
	 * 2. copy rel_sel_ti into newly allocated
	 * 2. relink newly allocated into rel_sel_ti reference
	 * 3. replace rel_sel_ti entry into free pool
	 *
	 * NOTE: relink and replace rely on *initial values* before reloc
	 * therefore it is imperative to keep wlogs for both and only
	 * commit after both have been computed
	 *
	 * Alloc must be committed before replace and relink. This ensures
	 * that things like free counts are properly reflected in replace.
	 */
	uint64_t	wpi_params[2];
	uint64_t	wpkt_params[16];	/* XXX: this should be max of wpkts */

	/* wpkt_alloc => (choice_idx) */
	wpi_params[0] = replace_choice_idx;
	rel->rel_alloc.wpi_params(&ti_clo(ti), wpi_params, wpkt_params);
	fsl_io_do_wpkt(rel->rel_alloc.wpi_wpkt, wpkt_params);
	FSL_WRITE_COMPLETE();

	assert (0 == 1 && "NEED TO DO COPY HERE");
	typeinfo_phys_copy(replace_choice_ti, rel_sel_ti);

	/* wpkt_replace => (sel_idx) */
	wpi_params[0] = sel_v;
	rel->rel_replace.wpi_params(&ti_clo(ti), wpi_params, wpkt_params);
	fsl_io_do_wpkt(rel->rel_replace.wpi_wpkt, wpkt_params);
	fsl_io_steal_wlog(fsl_get_io(), &wlog_replace);

	/* wpkt_relink => (sel_idx, choice_idx) */
	wpi_params[0] = sel_v;
	wpi_params[1] = replace_choice_idx;
	rel->rel_relink.wpi_params(&ti_clo(ti), wpi_params, wpkt_params);
	fsl_io_do_wpkt(rel->rel_replace.wpi_wpkt, wpkt_params);
	FSL_WRITE_COMPLETE();

	fsl_wlog_commit(&wlog_replace);
}

void do_rel_type(struct type_info* ti, const struct fsl_rt_table_reloc* rel)
{
	struct type_info		*rel_new_ti;
	const struct fsl_rt_table_type	*dst_type;
	int				sel_cur, sel_min, sel_max;
	int				choice_min, choice_max;

	if (ccache == NULL) ccache_load(ti, rel);

	dst_type = tt_by_num(rel->rel_sel.it_type_dst);
	sel_min = rel->rel_sel.it_min(&ti_clo(ti));
	sel_max = rel->rel_sel.it_max(&ti_clo(ti));

	for (sel_cur = sel_min; sel_cur <= sel_max; sel_cur++) {
		struct type_info		*rel_sel_ti;

		rel_sel_ti = typeinfo_follow_iter(ti, &rel->rel_sel, sel_cur);
		swap_rel_sel(ti, rel, rel_sel_ti, sel_cur);
		typeinfo_free(rel_sel_ti);
	}
}

int ti_handle(struct type_info* ti, struct relocscan_info* rinfo)
{
	const struct fsl_rt_table_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	if (tt->tt_reloc_c == 0)
		return SCAN_RET_CONTINUE;

	for (i = 0; i < tt->tt_reloc_c; i++)
		do_rel_type(ti, &tt->tt_reloc[i]);

	rinfo->rel_c++;

	return SCAN_RET_CONTINUE;
}

struct scan_ops ops = { .so_ti = (scan_ti_f)ti_handle };

/* 8 bits per bit-- haters gonna hate */
struct img* img_load(const char* path, uint64_t disk_bytes)
{
	FILE		*f;
	struct img	*im;
	int		x, y;
	int		ret;

	f = fopen(path, "r");
	assert (f != NULL);
	im = malloc(sizeof(struct img));
	im->disk_bytes = disk_bytes;

	ret = fscanf(f, "%d", &im->x);
	assert (ret > 0);
	ret = fscanf(f, "%d", &im->y);
	assert (ret > 0);

	printf("%d %d\n", im->x, im->y);
	im->bmp = malloc(im->x*im->y);
	for (y = 0; y < im->y; y++) {
		for (x = 0; x < im->x; x++) {
			int	v;
			ret = fscanf(f, "%d", &v);
			assert (ret > 0);
			im->bmp[x+im->x*y] = v;
		}
	}

	return im;
}

void img_free(struct img* im)
{
	free(im->bmp);
	free(im);
}

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;
	struct type_desc	init_td = td_origin();
	struct relocscan_info	info;
	unsigned int 		i;

	printf("Welcome to fsl relocate. Filesystem mode: \"%s\"\n", fsl_rt_fsname);
	assert (argc > 0 && "./relocate hd.img img.bmp");
	printf("Loading image %s\n", argv[0]);
	reloc_img = img_load(argv[0], __FROM_OS_BDEV_BYTES);
	printf("OK. im=(%d,%d)\n", reloc_img->x, reloc_img->y);

	reloc_cursor = ~0;
	reloc_img_advance_unchecked();

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_by_field(&init_td, NULL, NULL);
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		return -1;
	}

	DEBUG_TOOL_WRITE("Origin Type Now Allocated");

	info.rel_c = 0;
	scan_type(origin_ti, &ops, &info);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}