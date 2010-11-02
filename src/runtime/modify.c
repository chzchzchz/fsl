//#define DEBUG_TOOL
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "runtime.h"
#include "debug.h"
#include "type_info.h"

static int lookup(struct type_info* cur_ti, const char* v);

static int lookup_fields(
	struct type_info* cur_ti, /* path prior car */
	const char* cur_elem,	/* path car */
	const char* next_elems	/* path cdr */);
static int lookup_virts(
	struct type_info* cur_ti, /* path prior car */
	const char* cur_elem,	/* path car */
	const char* next_elems	/* path cdr */);
static int lookup_pointstos(
	struct type_info* cur_ti, /* path prior car */
	const char* cur_elem,	/* path car */
	const char* next_elems	/* path cdr */);

/* return pointer to path-expr past the head element */
static const char* chop_head(const char* v)
{
	char	*dot_loc;

	dot_loc = strchr(v, '.');
	if (dot_loc == NULL)
		return NULL;

	while (*dot_loc == '.' && *dot_loc != '\0')
		dot_loc++;

	if (*dot_loc == '\0') return NULL;

	return dot_loc;
}

static bool get_idx(const char* cur, int* idx)
{
	char	*next_bracket;
	char	c;
	int	ret;

	next_bracket = strchr(cur, '[');
	if (next_bracket == NULL)
		return false;

	next_bracket++;
	c = *next_bracket;
	ret = 0;
	while (c != '\0' && c != ']') {
		if (c > '9' || c < '0')
			return false;

		ret *= 10;
		ret += c - '0';
	}

	if (idx) *idx = ret;

	return true;
}

static char* get_name(const char* cur)
{
	char	*bracket;
	char	*ret;

	assert (cur != NULL);

	bracket = strchr(cur, '[');
	if (bracket == NULL)
		return strdup(cur);

	ret = strdup(cur);
	ret[bracket - cur] = '\0';

	return ret;
}

/* allocated null terminated string */
static char* get_cur(const char* v)
{
	char	*next_dot, *ret;
	int	dot_off;

	if (v == NULL) return NULL;

	next_dot = strchr(v, '.');
	if (next_dot == NULL)
		return strdup(v);

	dot_off = v - next_dot;

	ret = strdup(next_dot);
	ret[dot_off] = '\0';	/* NUL it */

	return ret;
}

static const struct fsl_rt_table_virt* find_virt(
	struct type_info* cur_ti,
	const char* fieldname)
{
	const struct fsl_rt_table_type	*tt;
	unsigned int			virt_idx;
	char				*fn;
	const struct fsl_rt_table_virt	*ret = NULL;

	tt = tt_by_ti(cur_ti);
	fn = get_name(fieldname);
	for (virt_idx = 0; virt_idx < tt->tt_virt_c; virt_idx++) {
		const struct fsl_rt_table_virt	*vt;
		vt = &tt->tt_virt[virt_idx];
		if (vt->vt_name && strcmp(fn, vt->vt_name) == 0) {
			/* found it */
			ret = vt;
			break;
		}

	}
	free(fn);

	return ret;
}

static int lookup_virts(
	struct type_info* cur_ti, /* path prior car */
	const char* cur_elem,	/* path car */
	const char* next_elems	/* path cdr */)
{
	const struct fsl_rt_table_virt*	vt;

	vt = find_virt(cur_ti, cur_elem);
	if (vt == NULL) return 0;

	assert (0 == 1 && "BUSTED");
}

static const struct fsl_rt_table_pointsto* find_pt(
	struct type_info* cur_ti,
	const char* fieldname)
{
	const struct fsl_rt_table_type	*tt;
	unsigned int			pt_idx;
	char				*fn;
	const struct fsl_rt_table_pointsto*	ret = NULL;

	tt = tt_by_ti(cur_ti);
	fn = get_name(fieldname);

	for (pt_idx = 0; pt_idx < tt->tt_pointsto_c; pt_idx++) {
		struct fsl_rt_table_pointsto *pt;

		pt = &tt->tt_pointsto[pt_idx];
		if (pt->pt_name && strcmp(fieldname, pt->pt_name) == 0) {
			/* found it */
			ret = pt;
			break;
		}

	}

	free(fn);
	return ret;
}

static int lookup_pointstos(
	struct type_info* cur_ti, /* path prior car */
	const char* cur_elem,	/* path car */
	const char* next_elems	/* path cdr */)
{
	const struct fsl_rt_table_pointsto	*pt;

	pt = find_pt(cur_ti, cur_elem);
	if (pt == NULL) return 0;

	assert (0 == 1 && "BUSTED");
}

static const struct fsl_rt_table_field* find_field(
	struct type_info* cur_ti,
	const char* fieldname)
{
	const struct fsl_rt_table_type	*tt;
	unsigned int			field_idx;
	char				*fn;
	struct fsl_rt_table_field	*ret = NULL;

	tt = tt_by_ti(cur_ti);
	fn = get_name(fieldname);

	for (field_idx = 0; field_idx < tt->tt_field_c; field_idx++) {
		struct fsl_rt_table_field	*tf;

		tf = &tt->tt_fieldall_thunkoff[field_idx];
		if (strcmp(fieldname, tf->tf_fieldname) == 0) {
			/* found it */
			ret = tf;
			break;
		}

	}

	free(fn);
	return ret;
}

static int lookup_fields(
	struct type_info* cur_ti, /* path prior car */
	const char* cur_elem,	/* path car */
	const char* next_elems	/* path cdr */)
{
	struct type_info		*next_ti;
	struct type_desc		next_td;
	const struct fsl_rt_table_field	*next_field;
	struct fsl_rt_table_type	*tt_next_field;
	uint64_t			next_field_off;
	unsigned int			field_idx;

	next_field = find_field(cur_ti, cur_elem);

	/* could not find name in fields */
	if (next_field == NULL) return 0;

	/* primitive type? */
	if (next_field->tf_typenum == TYPENUM_INVALID) {
		/* physical.. no typethunks */
		if (next_elems != NULL) {
			printf("OOPS. Path doesn't terminate on primitive.");
			return -1;
		}
		printf("HIT THE PHYSICAL.");
		assert (0 == 1 && "Can not purse physical types");
	}

	tt_next_field = tt_by_num(next_field->tf_typenum);
	uint64_t next_field_params[tt_next_field->tt_param_c];

	next_field_off = next_field->tf_fieldbitoff(&ti_clo(cur_ti));
	next_field->tf_params(&ti_clo(cur_ti), 0, next_field_params);

	td_init(&next_td,
		next_field->tf_typenum,
		next_field_off,
		next_field_params);

	next_ti = typeinfo_alloc_by_field(&next_td, next_field, cur_ti);
	if (next_ti == NULL) return -1;

	return lookup(next_ti, next_elems);
}

static int lookup(struct type_info* cur_ti, const char* v)
{
	char				*cur_elem;
	const char			*rest_elems;
	int				ret;

	if (cur_ti == NULL) return -1;
	if (v == NULL) return -1;

	cur_elem = get_cur(v);
	if (cur_elem == NULL) return -1;

	rest_elems = chop_head(v);
	if ((ret = lookup_fields(cur_ti, cur_elem, rest_elems)) > 0)
		goto done;
	else if ((ret = lookup_pointstos(cur_ti, cur_elem, rest_elems)) > 0)
		goto done;
	else if ((ret = lookup_virts(cur_ti, cur_elem, rest_elems)) > 0)
		goto done;

	ret = -1;
	free(cur_elem);

done:
	if (ret > 0) ret = 0;

	return ret;
}

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;
	struct type_desc	init_td = td_origin();
	char			*var_name, *first_entry;
	int			ret;

	printf("Welcome to fsl modify. Modify mode: \"%s\"\n", fsl_rt_fsname);

	if (argc < 1) {
		printf("Expected one argument (varname)\n");
		return -2;
	}


	origin_ti = typeinfo_alloc_by_field(&init_td, NULL, NULL);
	if (origin_ti == NULL) {
		printf("Could not open origin type\n");
		return -1;
	}

	var_name = argv[0];
	first_entry = get_cur(var_name);
	if (first_entry == NULL ||
	    strcmp(first_entry, tt_by_ti(origin_ti)->tt_name) != 0)
	{
		printf("Must match origin type, got '%s'\n", first_entry);
		free(first_entry);
		return -3;
	}
	free(first_entry);

	ret = lookup(origin_ti, chop_head(var_name));
	printf("Finding %s\n", var_name);
	assert (0 == 1 && "NOT DONE YET");

	typeinfo_free(origin_ti);
	printf("Have a nice day.\n");

	return ret;
}