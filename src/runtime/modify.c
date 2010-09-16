#include "runtime.h"
#include "type_info.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int lookup(struct type_info* cur_type, const char* v);

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

/* allocated null terinated string */
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
#if 0
static void lookup_pointstos
static void lookup_virt(
	struct type_info* cur_type,
#endif

static int lookup_fields(
	struct type_info* cur_type,
	const char* cur_elem, const char* next_elems)
{
	struct type_info		*next_ti;
	struct type_desc		*next_td;
	struct fsl_rt_table_field	*next_field;
	struct fsl_rt_table_type	*tt_next_field;
	struct fsl_rt_table_type	*tt;
	uint64_t			next_field_off;
	unsigned int			field_idx;

	tt = tt_by_ti(cur_type);
	next_field = NULL;
	for (field_idx = 0; field_idx < tt->tt_fieldall_c; field_idx++) {
		struct fsl_rt_table_field	*tf;

		tf = &tt->tt_fieldall_thunkoff[field_idx];
		if (strcmp(cur_elem, tf->tf_fieldname) == 0) {
			/* found it */
			next_field = tf;
			break;
		}
	}

	if (next_field == NULL) {
		printf("Could not find '%s'\n", cur_elem);
		return 0;
	}

	if (next_field->tf_typenum == TYPENUM_INVALID) {
		/* physical.. no typethunks */
		if (next_elems != NULL) {
			printf("OOPS. Path doesn't terminate on primitive.");
			return -1;
		}
		printf("HIT THE PHYSICAL.");
		assert (0 == 1);
	}

	tt_next_field = tt_by_num(next_field->tf_typenum);
	uint64_t next_field_params[tt_next_field->tt_param_c];

	next_field_off = next_field->tf_fieldbitoff(&ti_clo(cur_type));
	next_field->tf_params(&ti_clo(cur_type), next_field_params);

	td_init(next_td, next_field->tf_typenum,
		next_field_off, next_field_params);

	next_ti = typeinfo_alloc(next_td, field_idx, cur_type);
	if (next_ti == NULL)
		return -1;

	return lookup(next_ti, next_elems);
}

static int lookup(struct type_info* cur_type, const char* v)
{
	char				*cur_elem;
	const char			*rest_elems;
	int				ret;

	if (cur_type == NULL) return -1;
	if (v == NULL) return -1;

	cur_elem = get_cur(v);
	if (cur_elem == NULL) return -1;

	rest_elems = chop_head(v);
	if ((ret = lookup_fields(cur_type, cur_elem, rest_elems)) > 0)
		goto done;
#if 0
/* not yet */
	if ((ret = lookup_pointsto(cur_type, cur_elem, rest_elems)) > 0)
		goto done;
	if ((ret = lookup_virts(cur_type, cur_elem, rest_elems)) > 0)
		goto done;
#endif
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


	origin_ti = typeinfo_alloc(&init_td, 0, NULL);
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