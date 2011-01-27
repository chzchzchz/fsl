//#define DEBUG_TYPEINFO
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "io.h"
#include "runtime.h"
#include "debug.h"
#include "type_info.h"

static void typeinfo_print_field(const struct type_info* ti);
static void typeinfo_print_default(const struct type_info* ti);
static void typeinfo_print_virt(const struct type_info* ti);
#define typeinfo_print_pointsto(x) typeinfo_print_default(x)
#define typeinfo_print_iter(x) typeinfo_print_default(x)

static void typeinfo_print_path_helper(const struct type_info* ti);
static void typeinfo_print_type(const struct type_info* ti);

static void typeinfo_print_path_helper(const struct type_info* ti)
{
	/* end of recursion? */
	if (ti == NULL)
		return;

	typeinfo_print_path_helper(ti->ti_prev);

	typeinfo_print(ti);
	printf("/");
}

void typeinfo_print_field_value(
	const struct type_info		*ti,
	const struct fsl_rtt_field	*field)
{
	uint64_t			num_elems;
	typesize_t			field_sz;
	diskoff_t			field_off;
	TI_INTO_CLO(ti);

	DEBUG_TYPEINFO_ENTER();

	/* print field name */
	num_elems = field->tf_elemcount(clo);


	field_sz = ti_field_size(ti, field, &num_elems);
	printf("%s", field->tf_fieldname);
	if (num_elems > 1) {
		printf("[%"PRIu64"]", num_elems);
	}

	DEBUG_TYPEINFO_WRITE("Width = %d", field_sz);

	field_off = field->tf_fieldbitoff(clo);
	NEW_VCLO(last_field_clo, field_off, NULL, ti_xlate(ti));

	/* dump data or print address range */
	if (num_elems == 1 && field_sz <= 64) {
		uint64_t	v;
		v = __getLocal(&last_field_clo, field_off, field_sz);
		printf(" = %"PRIu64 " (0x%"PRIx64")", v, v);
	}

#ifdef PRINT_BITS
	printf("@%"PRIu64"--%"PRIu64, field_off, field_off + field_sz);
#else
	printf("@%"PRIu64"--%"PRIu64, field_off/8, (field_off + field_sz)/8);
#endif

	printf("\n");

	DEBUG_TYPEINFO_LEAVE();
}

static void typeinfo_print_type(const struct type_info* ti)
{
	const struct fsl_rtt_type	*tt;
	uint64_t			len;
	TI_INTO_CLO			(ti);

	tt = tt_by_ti(ti);
	len = tt->tt_size(clo);

#ifdef PRINT_BITS
	printf("%s@%"PRIu64"--%"PRIu64, //" (%"PRIu64" bits)",
		tt_by_ti(ti)->tt_name,
		ti_offset(ti),
		ti_offset(ti) + len //,
//		len
	);
#else
	printf("%s@%"PRIu64"--%"PRIu64, //" (%"PRIu64" bytes)",
		tt_by_ti(ti)->tt_name,
		ti_offset(ti)/8,
		(ti_offset(ti) + len)/8 //,
//		len/8
	);
#endif
}

static void typeinfo_print_field(const struct type_info* ti)
{
	const struct fsl_rtt_type	*tt;
	const struct fsl_rtt_field	*field;
	uint64_t			len;
	TI_INTO_CLO			(ti->ti_prev);

	field = ti->ti_field;
	assert (field != NULL);

	tt = tt_by_ti(ti->ti_prev);
	len = field->tf_typesize(clo);
#ifdef PRINT_BITS
	printf("%s.%s@%"PRIu64"--%"PRIu64,
		tt->tt_name,
		field->tf_fieldname,
		ti_offset(ti),
		ti_offset(ti) + len
	);
#else
	printf("%s.%s@%"PRIu64"--%"PRIu64,
		tt->tt_name,
		field->tf_fieldname,
		ti_offset(ti)/8,
		(ti_offset(ti) + len)/8
	);
#endif
}

static void typeinfo_print_default(const struct type_info* ti)
{
	assert (ti->ti_print_name != NULL);
	if (ti->ti_print_idxval != TI_INVALID_IDXVAL)
		printf("%s[%"PRIu64"]", ti->ti_print_name, ti->ti_print_idxval);
	else
		printf("%s", ti->ti_print_name);
#ifdef PRINT_BITS
	printf("@%"PRIu64, ti_offset(ti));
#else
	printf("@%"PRIu64, ti_offset(ti) / 8);
#endif
}

static void typeinfo_print_virt(const struct type_info* ti)
{
	assert (ti->ti_print_name != NULL);
	assert (ti_xlate(ti) != NULL);

	if (ti->ti_print_idxval != TI_INVALID_IDXVAL)
		printf("%s[%"PRIu64"]", ti->ti_print_name, ti->ti_print_idxval);
	else
		printf("%s", ti->ti_print_name);
#ifdef PRINT_BITS
	printf("@v%"PRIu64"p%"PRIu64,
		ti_offset(ti),
		fsl_virt_xlate(&ti_clo(ti), ti_offset(ti)));
#else
	printf("@v%"PRIu64"p%"PRIu64,
		ti_offset(ti) / 8,
		fsl_virt_xlate(&ti_clo(ti), ti_offset(ti)) / 8);
#endif
}

void typeinfo_print(const struct type_info* ti)
{
	assert (ti != NULL);

	if (ti->ti_prev == NULL && ti_typenum(ti) != TYPENUM_INVALID) {
		typeinfo_print_type(ti);
		return;
	}

	assert (ti->ti_prev != NULL);

	if (ti->ti_field) typeinfo_print_field(ti);
	else if (ti->ti_points) typeinfo_print_pointsto(ti);
	else if (ti->ti_virt) typeinfo_print_virt(ti);
	else if (ti->ti_iter) typeinfo_print_iter(ti);
	else printf("???");
}

void typeinfo_print_fields(const struct type_info* ti)
{
	const struct fsl_rtt_type	*tt;
	unsigned int			i;

	if (ti_typenum(ti) == TYPENUM_INVALID)
		return;

	tt = tt_by_ti(ti);

	if (tt->tt_fieldall_c > 0)
		printf("Fields:\n");

	for (i = 0; i < tt->tt_fieldall_c; i++) {
		const struct fsl_rtt_field	*field;
		bool				is_present;
		TI_INTO_CLO			(ti);

		field = &tt->tt_fieldall_thunkoff[i];
		if (field->tf_cond == NULL)
			is_present = true;
		else
			is_present = field->tf_cond(clo);

		if (is_present == false)
			continue;

		if (field->tf_typenum != TYPENUM_INVALID)
			printf("%02d. ", i);
		else
			printf("--- ");

		typeinfo_print_field_value(ti, field);
	}
}

void typeinfo_print_path(const struct type_info* ti)
{
	typeinfo_print_path_helper(ti->ti_prev);
	typeinfo_print(ti);
}

void typeinfo_print_pointstos(const struct type_info* ti)
{
	const struct fsl_rtt_type	*tt;
	bool				none_seen;
	unsigned int			i;

	if (ti_typenum(ti) == TYPENUM_INVALID)
		return;

	tt = tt_by_ti(ti);

	none_seen = true;
	for (i = 0; i < tt->tt_pointsto_c; i++) {
		const struct fsl_rtt_pointsto	*pt;
		uint64_t			pt_min, pt_max;
		TI_INTO_CLO			(ti);

		pt = &tt->tt_pointsto[i];
		pt_min = pt->pt_iter.it_min(clo);
		if (pt_min == ~0) continue;
		pt_max = pt->pt_iter.it_max(clo);
		if (pt_min > pt_max) continue;

		if (none_seen) {
			printf("Points-To:\n");
			none_seen = false;
		}

		if (pt->pt_name == NULL) {
			printf("%02d. (%s)\n",
				tt->tt_fieldall_c + i,
				tt_by_num(pt->pt_iter.it_type_dst)->tt_name);
		} else {
			printf("%02d. %s (%s)\n",
				tt->tt_fieldall_c + i,
				pt->pt_name,
				tt_by_num(pt->pt_iter.it_type_dst)->tt_name);
		}

	}
}

void typeinfo_print_virts(const struct type_info* ti)
{
	const struct fsl_rtt_type	*tt;
	unsigned int			i;
	bool				none_seen;

	if (ti_typenum(ti) == TYPENUM_INVALID) return;

	DEBUG_TYPEINFO_ENTER();

	tt = tt_by_ti(ti);

	none_seen = true;
	for (i = 0; i < tt->tt_virt_c; i++) {
		const struct fsl_rtt_virt	*vt;
		uint64_t			vt_min, vt_max;
		TI_INTO_CLO			(ti);

		vt = &tt->tt_virt[i];
		DEBUG_TYPEINFO_WRITE("Type: %s. xlate=%p. voff=%"PRIu64". Virt=%s/%d",
			tt->tt_name,
			clo->clo_xlate,
			ti_offset(ti),
			vt->vt_name, i);

		vt_min = vt->vt_iter.it_min(clo);
		if (vt_min == ~0) continue;
		vt_max = vt->vt_iter.it_max(clo);
		if (vt_min > vt_max) continue;

		if (none_seen) {
			printf("Virtuals: \n");
			none_seen = false;
		}

		if (vt->vt_name == NULL) {
			printf("%02d. (%s->%s)\n",
				tt->tt_fieldall_c + tt->tt_pointsto_c + i,
				tt_by_num(vt->vt_iter.it_type_dst)->tt_name,
				tt_by_num(vt->vt_type_virttype)->tt_name);
		} else {
			printf("%02d. %s (%s->%s)\n",
				tt->tt_fieldall_c + tt->tt_pointsto_c + i,
				vt->vt_name,
				tt_by_num(vt->vt_iter.it_type_dst)->tt_name,
				tt_by_num(vt->vt_type_virttype)->tt_name);
		}

	}

	DEBUG_TYPEINFO_LEAVE();
}

#define DUMP_WIDTH	0x18
static char hexmap[] = {"0123456789abcdef"};

void typeinfo_dump_data(const struct type_info* ti)
{
	uint64_t			type_sz;
	unsigned int			i;
	TI_INTO_CLO			(ti);

	type_sz = tt_by_ti(ti)->tt_size(clo);

	for (i = 0; i < type_sz / 8; i++) {
		uint8_t	c;

		if ((i % DUMP_WIDTH) == 0) {
			if (i != 0) printf("\n");
			printf("%04x: ", i);
		}
		c = __getLocal(clo, ti_offset(ti) + (i*8), 8);
		printf("%c%c ", hexmap[(c >> 4) & 0xf], hexmap[c & 0xf]);

	}

	printf("\n");
	if (type_sz % 8)
		printf("OOPS-- size is not byte-aligned\n");
}

void typeinfo_print_value(const struct type_info* ti)
{
	assert (ti != NULL);
	assert (ti->ti_field != NULL && "Only fields supported");
	typeinfo_print_field_value(ti->ti_prev, ti->ti_field);
}
