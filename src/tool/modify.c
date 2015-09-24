//#define DEBUG_TOOL
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "runtime.h"
#include "debug.h"
#include "type_info.h"

#define set_err(x,y)	do { if ((x) != NULL) (*x) = y; } while (0)
#define ret_err(x,y)	do { set_err(x,y); return NULL; } while (0)

#define LOOKUP_RC_UNEXPECT_PRIM	(-8)
#define LOOKUP_RC_NOPARENT	(-7)
#define LOOKUP_RC_BADPATH	(-6)
#define LOOKUP_RC_NOTFOUND	(-5)
#define LOOKUP_RC_BADFIELD	(-4)
#define LOOKUP_RC_NOPOINTSTO	(-3)
#define LOOKUP_RC_NOVIRT	(-2)
#define LOOKUP_RC_NOFIELD	(-1)
#define LOOKUP_RC_OK		(0)

static struct type_info* lookup(
	struct type_info* cur_ti, const char* v, int* err);
static struct type_info* __lookup(
	struct type_info* cur_ti, const char* v, int* err);

static struct type_info* lookup_fields(
	struct type_info*	cur_ti, /* path prior car */
	const char*		cur_elem,	/* path car */
	const char* 		next_elems,	/* path cdr */
	int*			err);
static struct type_info* lookup_virts(
	struct type_info*	cur_ti, /* path prior car */
	const char*		cur_elem,	/* path car */
	const char*		next_elems,	/* path cdr */
	int*			err);
static struct type_info* lookup_pointstos(
	struct type_info*	cur_ti, /* path prior car */
	const char*		cur_elem,	/* path car */
	const char*		next_elems,	/* path cdr */
	int*			err);

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

#if 0
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
#endif

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
static char* get_head(const char* v)
{
	char	*next_dot, *ret;
	int	dot_off;

	if (v == NULL) return NULL;

	next_dot = strchr(v, '.');
	if (next_dot == NULL)
		return strdup(v);

	dot_off = next_dot - v;

	ret = strdup(v);
	ret[dot_off] = '\0';	/* NUL it */

	return ret;
}

static const struct fsl_rtt_virt* find_virt(
	struct type_info* cur_ti,
	const char* fieldname)
{
	const struct fsl_rtt_type	*tt;
	unsigned int			virt_idx;
	char				*fn;
	const struct fsl_rtt_virt	*ret = NULL;

	tt = tt_by_ti(cur_ti);
	fn = get_name(fieldname);
	for (virt_idx = 0; virt_idx < tt->tt_virt_c; virt_idx++) {
		const struct fsl_rtt_virt	*vt;
		vt = &tt->tt_virt[virt_idx];
		if (vt->vt_name && strcmp(fn, vt->vt_name) == 0) {
			ret = vt;
			break;
		}

	}
	free(fn);

	return ret;
}

static struct type_info* lookup_virts(
	struct type_info* cur_ti, /* path prior car */
	const char* cur_elem,	/* path car */
	const char* next_elems,	/* path cdr */
	int* err)
{
	struct type_info		*next_ti = cur_ti;
	const struct fsl_rtt_virt	*vt;

	vt = find_virt(cur_ti, cur_elem);
	if (vt == NULL) ret_err(err, LOOKUP_RC_NOVIRT);

	return lookup(next_ti, next_elems, err);
}

static const struct fsl_rtt_pointsto* find_pt(
	struct type_info* cur_ti,
	const char* fieldname)
{
	const struct fsl_rtt_type	*tt;
	unsigned int			pt_idx;
	char				*fn;
	const struct fsl_rtt_pointsto*	ret = NULL;

	tt = tt_by_ti(cur_ti);
	fn = get_name(fieldname);

	for (pt_idx = 0; pt_idx < tt->tt_pointsto_c; pt_idx++) {
	const struct fsl_rtt_pointsto *pt;

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

static struct type_info* lookup_pointstos(
	struct type_info* cur_ti, /* path prior car */
	const char* cur_elem,	/* path car */
	const char* next_elems,	/* path cdr */
	int* err)
{
	struct type_info			*next_ti;
	const struct fsl_rtt_pointsto	*pt;

	pt = find_pt(cur_ti, cur_elem);
	if (pt == NULL) ret_err(err, LOOKUP_RC_NOPOINTSTO);

	/* XXX: need to be able to pull in idx!=0*/
	next_ti = typeinfo_follow_pointsto(cur_ti, pt, 0);

	return lookup(next_ti, next_elems, err);
}

static const struct fsl_rtt_field* find_field(
	struct type_info* cur_ti,
	const char* fieldname)
{
	const struct fsl_rtt_type	*tt;
	unsigned int			field_idx;
	char				*fn;
	const struct fsl_rtt_field	*ret = NULL;

	tt = tt_by_ti(cur_ti);
	fn = get_name(fieldname);

	for (field_idx = 0; field_idx < tt->tt_field_c; field_idx++) {
		const struct fsl_rtt_field	*tf;

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

static struct type_info* lookup_fields(
	struct type_info* cur_ti, /* path prior car */
	const char* cur_elem,	/* path car */
	const char* next_elems,	/* path cdr */
	int*	err)
{
	struct type_info		*next_ti;
	const struct fsl_rtt_field	*next_field;

	next_field = find_field(cur_ti, cur_elem);

	/* could not find name in fields */
	if (next_field == NULL) ret_err(err, LOOKUP_RC_NOFIELD);

	/* primitive type? */
	if (next_field->tf_typenum == TYPENUM_INVALID) {
		/* physical.. no typethunks */

		// Path should always terminate on primitive.
		if (next_elems != NULL) ret_err(err, LOOKUP_RC_UNEXPECT_PRIM);

//		typeinfo_print_field_value(cur_ti, next_field);
		return typeinfo_follow_field(cur_ti, next_field);
	}

	next_ti = typeinfo_follow_field(cur_ti, next_field);
	if (next_ti == NULL) ret_err(err, LOOKUP_RC_BADFIELD);

	return lookup(next_ti, next_elems, err);
}

static struct type_info* __lookup(
	struct type_info* cur_ti, const char* v, int *err)
{
	char				*cur_elem;
	const char			*rest_elems;
	struct type_info		*ret;

	assert (v != NULL && "Don't pass null into lookup");
	assert (cur_ti != NULL && "No parent");

	cur_elem = get_head(v);
	if (cur_elem == NULL) ret_err(err, LOOKUP_RC_BADPATH);

	rest_elems = chop_head(v);
	if ((ret = lookup_fields(cur_ti, cur_elem, rest_elems, err)) != NULL)
		goto done;
	else if ((ret = lookup_pointstos(cur_ti, cur_elem, rest_elems, err)) != NULL)
		goto done;
	else if ((ret = lookup_virts(cur_ti, cur_elem, rest_elems, err)) != NULL)
		goto done;

done:
	/* lookup is passed ownership of cur_elem */
	if (ret != NULL)
		set_err(err, LOOKUP_RC_OK);

	free(cur_elem);

	return ret;
}

/* takes ownership of cur_ti */
static struct type_info* lookup(
	struct type_info* cur_ti, const char* v, int *err)
{
	struct type_info	*ret;

	ret = __lookup(cur_ti, v, err);
	typeinfo_free(cur_ti);

	return ret;
}

int tool_entry(int argc, char* argv[])
{
	struct type_info	*origin_ti;
	struct type_info	*lookup_ti;
	struct type_desc	init_td = td_origin();
	char			*var_name, *first_entry;
	int			err;

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
	first_entry = get_head(var_name);
	if (first_entry == NULL ||
	    strcmp(first_entry, tt_by_ti(origin_ti)->tt_name) != 0)
	{
		printf("Must match origin type, got '%s'\n", first_entry);
		free(first_entry);
		return -3;
	}
	free(first_entry);

	printf("Finding %s\n", var_name);
	lookup_ti = __lookup(origin_ti, chop_head(var_name), &err);
	if (lookup_ti == NULL) {
		fprintf(stderr, "Error finding %s: err=%d\n", var_name, err);
	} else {
		printf("Found it!...\n");
		typeinfo_print(lookup_ti);
		printf("\n");
		typeinfo_print_value(lookup_ti);
		printf("\n");
	}

	if (lookup_ti != NULL) typeinfo_free(lookup_ti);
	typeinfo_free(origin_ti);
	printf("Have a nice day.\n");

	return err;
}