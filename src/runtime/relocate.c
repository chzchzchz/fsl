//#define DEBUG_TOOL
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "runtime.h"
#include "debug.h"
#include "type_info.h"
#include "bitmap.h"
#include "scan.h"
#include "log.h"
#include "choice.h"
#include "writepkt.h"

#define RELNAME_RELOC	"reloc"

struct img
{
	unsigned int	x;
	unsigned int	y;
	uint8_t		*bmp;
	uint64_t	disk_bytes;
	uint8_t		*bmp_disk;
};

struct relocscan_info { unsigned int	rel_c; };

#define bytes_per_px(a)	((a)->disk_bytes/((a)->x*(a)->y))

static int byte_to_pixval(struct img* im, diskoff_t in_byte)
{
	assert ((in_byte/bytes_per_px(im)) < (im->x*im->y));
	return im->bmp[in_byte/bytes_per_px(im)];
}

static uint64_t byte_to_imgidx(struct img* im, diskoff_t in_byte)
{
	return in_byte/bytes_per_px(im);
}

static struct choice_cache	*ccache = NULL;
static struct img		*reloc_img;
static uint64_t			ccache_cursor;
static uint64_t			reloc_cursor;
static int			reloc_fail_c = 0;

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

static void ccache_load(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	ccache = choice_alloc(ti, rel);
	ccache_cursor = choice_min(ccache);
}

/* finds the next choice that falls on reloc_cursor
 * that is not before the ccache_cursor
 * We need to say that we need choices to hold to some invariant
 * for this to work
 */
uint64_t choice_find(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	unsigned int	cur_choice;
	unsigned int	param_c;

	param_c = tt_by_num(rel->rel_choice.it_type_dst)->tt_param_c;
	for (	cur_choice = ccache_cursor;
		cur_choice <= choice_max(ccache);
		cur_choice++)
	{
		uint64_t	buf[param_c];
		diskoff_t	off_bits;
		uint64_t	imgidx;

		/* check cache first-- don't use allocated type */
		if (choice_is_alloc(ccache, cur_choice)) continue;

		/* get offset on disk of choice type */
		off_bits = rel->rel_choice.it_range(&ti_clo(ti), cur_choice, buf);
		imgidx = byte_to_imgidx(reloc_img, off_bits / 8);
		if (imgidx == reloc_cursor) {
			/* success! */
			choice_mark_alloc(ccache, cur_choice);
			reloc_img_advance();
			if (reloc_cursor == ~0) {
				/* nothing left to do */
				ccache_cursor = choice_max(ccache) + 1;
			} else
				ccache_cursor = cur_choice + 1;
			return cur_choice;
		} else if (imgidx > reloc_cursor) {
			/* overshot.. can't alloc */
		#ifdef DEBUG_TOOL
			printf("OOPS! Can't alloc at imgidx: %"PRIu64"\n",
				reloc_cursor);
		#endif
			reloc_fail_c++;
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

	return ~0;
}

/* compare location of selected with reloc locations */
/* if reloc location needs to be set, try to swap in */
/* if rel_sel location is on empty reloc location, try to swap out */
void swap_rel_sel(
	struct type_info* ti, const struct fsl_rtt_reloc* rel,
	struct type_info* rel_sel_ti, unsigned int sel_v)
{
	diskoff_t		sel_off_bit;
	uint64_t		replace_choice_idx;
	int			pxval;

	sel_off_bit = ti_offset(rel_sel_ti);
	pxval = byte_to_pixval(reloc_img, sel_off_bit/8);

	/* already in an OK place, don't do anything
	 * (in future, may want to steal if we have overlaps) */
	if (pxval != 0) return;

	DEBUG_TOOL_ENTER();

	/* need to move this to an unused location that
	 * is marked in the image */
	DEBUG_TOOL_WRITE("Need to move byteoff=%"PRIu64, sel_off_bit/8);
	replace_choice_idx = choice_find(ti, rel);
	if (replace_choice_idx == ~0) {
		DEBUG_TOOL_WRITE("No more moves left. Oh well.\n");
		return;
	}

	DEBUG_TOOL_WRITE("exchanging: sel_v=%d with choice_v=%"PRIu64,
		sel_v, replace_choice_idx);

	wpkt_relocate(ti, rel, rel_sel_ti, sel_v, replace_choice_idx);

	DEBUG_TOOL_LEAVE();
}

static void update_status(void)
{
	static int last_reloc_cursor = 0;
	static int last_percent = 999;
	int	percent;

	if (reloc_cursor == last_reloc_cursor) return;
	if (reloc_cursor == ~0) percent = 100;
	else percent = (100*reloc_cursor)/(reloc_img->x*reloc_img->y);
	if (percent == last_percent) return;

	printf("Status: %3d%%\r", percent);
	last_reloc_cursor = reloc_cursor;
	last_percent = percent;
}

void do_rel_type(struct type_info* ti, const struct fsl_rtt_reloc* rel)
{
	const struct fsl_rtt_type	*dst_type;
	int				sel_cur, sel_min, sel_max;

	if (ccache == NULL) ccache_load(ti, rel);

	dst_type = tt_by_num(rel->rel_sel.it_type_dst);
	sel_min = rel->rel_sel.it_min(&ti_clo(ti));
	sel_max = rel->rel_sel.it_max(&ti_clo(ti));

	DEBUG_TOOL_WRITE(
		"DO_REL_TYPE voff=%"PRIu64
		" poff=%"PRIu64" sel_min=%d // sel_max=%d",
		ti_offset(ti), ti_phys_offset(ti),
		sel_min, sel_max);

	for (sel_cur = sel_min; sel_cur <= sel_max; sel_cur++) {
		struct type_info		*rel_sel_ti;

		rel_sel_ti = typeinfo_follow_iter(ti, &rel->rel_sel, sel_cur);
		if (rel_sel_ti == NULL) {
			DEBUG_TOOL_WRITE("Failed to get rel_sel_ti for idx=%d\n",
				sel_cur);
			continue;
		}
		swap_rel_sel(ti, rel, rel_sel_ti, sel_cur);
		typeinfo_free(rel_sel_ti);
	}
	update_status();
}

int ti_handle(struct type_info* ti, struct relocscan_info* rinfo)
{
	const struct fsl_rtt_type	*tt;
	unsigned int			i;

	tt = tt_by_ti(ti);
	if (tt->tt_reloc_c == 0)
		return SCAN_RET_CONTINUE;

	for (i = 0; i < tt->tt_reloc_c; i++) {
		const struct fsl_rtt_reloc	*rel = &tt->tt_reloc[i];
		if (rel->rel_name == NULL) continue;
		if (strcmp(rel->rel_name, RELNAME_RELOC) != 0) continue;
		do_rel_type(ti, rel);
	}

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
	struct relocscan_info	info;

	printf("Welcome to fsl relocate. Filesystem mode: \"%s\"\n", fsl_rt_fsname);
	assert (argc > 0 && "./relocate hd.img img.bmp");
	printf("Loading image %s\n", argv[0]);
	reloc_img = img_load(argv[0], __FROM_OS_BDEV_BYTES);
	printf("OK. im=(%d,%d)\n", reloc_img->x, reloc_img->y);

	reloc_cursor = ~0;
	reloc_img_advance_unchecked();

	DEBUG_TOOL_WRITE("Origin Type Allocating...\n");
	origin_ti = typeinfo_alloc_origin();
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		printf("Failed assert: %s\n", fsl_env->fctx_failed_assert);
		return -1;
	}

	DEBUG_TOOL_WRITE("Origin Type Now Allocated");

	info.rel_c = 0;
	scan_type(origin_ti, &ops, &info);

	if (ccache != NULL) choice_free(ccache);

	typeinfo_free(origin_ti);

	printf("Have a nice day\n");
	return 0;
}