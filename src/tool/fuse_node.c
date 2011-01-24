#include "fuse_node.h"
#include "type_info.h"
#include "runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

static struct type_info* ti_from_rootpath(const char* path);
static struct type_info* ti_from_relpath(
	struct type_info* ti_parent, const char* path);
static struct type_info* ti_from_parentpath(const char* path);
static char* get_path_child(const char* path);
static bool is_int(const char* s);

extern FILE* out_file;

static struct type_info* ti_from_rootpath(const char* path)
{
	struct type_info	*ti_origin, *ret;

	ti_origin = typeinfo_alloc_origin();
	if (ti_origin == NULL) return NULL;
	ret = ti_from_relpath(ti_origin, path);
	if (ret != ti_origin) typeinfo_free(ti_origin);

	return ret;
}

static struct type_info* ti_from_relpath(
	struct type_info* ti_parent, const char* path)
{
	struct type_info	*cur_ti = NULL, *ret = NULL;
	const char		*s = path, *s_next;
	char			*elem;

	while (*s == '/') s++;
	if (*s == '\0') return ti_parent;
	s_next = strchr(s, '/');
	if (s_next == NULL) s_next = s + strlen(s);

	elem = strndup(s, s_next - s);
	if (is_int(elem)) {
		/* handle array */
		uint64_t	idx, max_idx;

		if (ti_parent == NULL) goto know_cur_ti;
		if (ti_parent->ti_prev == NULL) goto know_cur_ti;

		idx = atoi(elem);
		max_idx = ti_parent->ti_field->tf_elemcount(
			&ti_clo(ti_parent->ti_prev));

		/* exceeded number of elements */
		if (max_idx <= idx) goto know_cur_ti;

		cur_ti = typeinfo_follow_field_off_idx(
			ti_parent->ti_prev,
			ti_parent->ti_field,
			ti_field_off(
				ti_parent->ti_prev,
				ti_parent->ti_field,
				idx),
			idx);
	} else
		cur_ti = typeinfo_lookup_follow(ti_parent, elem);

know_cur_ti:
	free(elem);
	if (cur_ti == NULL) return NULL;

	ret = ti_from_relpath(cur_ti, s_next);
	if (ret != cur_ti) typeinfo_free(cur_ti);

	return ret;
}

static struct type_info* ti_from_parentpath(const char* path)
{
	int			last_sep;
	char			*trunc_path;
	struct type_info	*ret;

	last_sep = strlen(path)-1;
	while (last_sep >= 0 && path[last_sep] == '/') last_sep--;
	if (last_sep < 0) return NULL;
	while (last_sep >= 0 && path[last_sep] != '/') last_sep--;
	if (last_sep < 0) return NULL;

	trunc_path = strndup(path, last_sep);
	ret = ti_from_rootpath(trunc_path);
	free(trunc_path);

	return ret;
}

static char* get_path_child(const char* path)
{
	int			last_sep, end_idx;

	last_sep = strlen(path)-1;
	while (last_sep >= 0 && path[last_sep] == '/') last_sep--;
	if (last_sep < 0) return NULL;
	end_idx = last_sep+1;

	while (last_sep >= 0 && path[last_sep] != '/') last_sep--;
	if (last_sep < 0) return NULL;
	last_sep++;

	return strndup(path+last_sep, end_idx - last_sep);
}

bool fslnode_by_primpath(
	const char* path, struct fsl_fuse_node* fn)
{

	struct type_info	*ti_parent;
	char			*child_path;

	ti_parent = ti_from_parentpath(path);
	if (ti_parent == NULL) return false;
	fn->fn_parent = ti_parent;

	child_path = get_path_child(path);
	fn->fn_field = fsl_lookup_field(tt_by_ti(ti_parent), child_path);
	free(child_path);
	fn->fn_prim_ti = typeinfo_follow_field(ti_parent, fn->fn_field);

	/* XXX: handle arrays */
	fn->fn_idx = 0;

	return true;
}

static bool is_int(const char* s)
{
	for(;*s;s++) { if (*s < '0' || *s > '9') return false; }
	return true;
}

static bool fslnode_by_arraypath(const char* path, struct fsl_fuse_node* fn)
{

	struct type_info	*ti_array_base;
	char			*child_path;
	int			idx, max_idx;
	bool			succ;

	succ = false;
	ti_array_base = ti_from_parentpath(path);
	if (ti_array_base == NULL) return false;
	if (ti_array_base->ti_prev == NULL) goto done;

	child_path = get_path_child(path);
	if (!is_int(child_path)) {
		free(child_path);
		goto done;
	}
	idx = atoi(child_path);
	max_idx = ti_array_base->ti_field->tf_elemcount(
		&ti_clo(ti_array_base->ti_prev));
	free(child_path);

	/* exceeded number of elements */
	if (max_idx <= idx) goto done;

	fn->fn_ti = typeinfo_follow_field_off_idx(
		ti_array_base->ti_prev,
		ti_array_base->ti_field,
		ti_field_off(
			ti_array_base->ti_prev,
			ti_array_base->ti_field,
			idx),
		idx);

	succ = true;
done:
	if (ti_array_base != NULL) typeinfo_free(ti_array_base);
	return succ;
}


struct fsl_fuse_node* fslnode_by_path(const char* path)
{
	struct fsl_fuse_node	*ret;

	ret = malloc(sizeof(*ret));
	if (ret == NULL) return NULL;

	memset(ret, 0, sizeof(*ret));
	ret->fn_ti = ti_from_rootpath(path);

	if (ret->fn_ti != NULL) {
		struct type_info		*ti_parent;
		const struct fsl_rtt_field	*tf;
		char				*path_child;

		path_child = get_path_child(path);
		if (path_child != NULL) {
			bool	is_elem_num;
			is_elem_num = is_int(path_child);
			free(path_child);
			if (is_elem_num) return ret;
		}

		tf = ret->fn_ti->ti_field;
		if (tf == NULL) return ret;

		ti_parent = ret->fn_ti->ti_prev;
		if (ti_parent == NULL) return ret;

		TI_INTO_CLO		(ti_parent);

		if (tf->tf_elemcount(clo) == 1) return ret;

		ret->fn_array_parent = ti_parent;
		typeinfo_ref(ti_parent);
		ret->fn_array_field = tf;

		typeinfo_free(ret->fn_ti);
		ret->fn_ti = NULL;

		/* array type */
		return ret;
	}

	if (fslnode_by_arraypath(path, ret)) goto done;
	if (fslnode_by_primpath(path, ret) == true) goto done;

	free(ret);
	return NULL;
done:
	return ret;

}

void fslnode_free(struct fsl_fuse_node* fn)
{
	if (fn->fn_ti != NULL) typeinfo_free(fn->fn_ti);
	if (fn->fn_parent != NULL) typeinfo_free(fn->fn_parent);
	if (fn->fn_array_parent != NULL) typeinfo_free(fn->fn_array_parent);
	if (fn->fn_prim_ti != NULL) typeinfo_free(fn->fn_prim_ti);
	free(fn);
}
