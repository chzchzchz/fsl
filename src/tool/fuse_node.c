#include "fuse_node.h"
#include "type_info.h"
#include "runtime.h"
#include "lookup.h"
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

static const char* get_path_next(const char* s)
{
	const char	*s_next;
	while (*s == '/') s++;
	if (*s == '\0') return NULL;
	s_next = strchr(s, '/');
	if (s_next == NULL) s_next = s + strlen(s);
	return s_next;
}

static char* get_path_first(const char* s)
{
	const char *s_next;
	while (*s == '/') s++;
	if (*s == '\0') return NULL;
	s_next = get_path_next(s);
	if (s_next == NULL) return NULL;
	return strndup(s, s_next - s);
}

static struct type_info* ti_from_relpath_idx(
	struct type_info* ti_p, const char* elem)
{
	uint64_t	idx, max_idx;

	if (ti_p == NULL) return NULL;
	if (ti_p->ti_prev == NULL) return NULL;

	idx = atoi(elem);
	max_idx = ti_p->ti_field->tf_elemcount(&ti_clo(ti_p->ti_prev));

	/* exceeded number of elements */
	if (max_idx <= idx) return NULL;

	return typeinfo_follow_field_off_idx(
		ti_p->ti_prev, ti_p->ti_field,
		ti_field_off(ti_p->ti_prev, ti_p->ti_field, idx),
		idx);
}

static struct type_info* ti_from_relpath_tryidx(
	struct type_info* ti_p, const char* elem, const char* s_next)
{
	struct type_info	*ret_ti;
	char			*next_elem;

	next_elem = get_path_first(s_next);
	if (next_elem == NULL) goto done;
	if (!is_int(next_elem)) goto cleanup;

	ret_ti = typeinfo_lookup_follow_idx(ti_p, elem, atoi(next_elem));
cleanup:
	free(next_elem);
done:
	return ret_ti;
}

static struct type_info* ti_from_relpath(
	struct type_info* ti_parent, const char* path)
{
	struct type_info	*cur_ti = NULL, *ret = NULL;
	const char		*s_next;
	char			*elem;

	s_next = get_path_next(path);
	if (s_next == NULL) return ti_parent;
	elem = get_path_first(path);
	if (elem == NULL) return ti_parent;

	if (is_int(elem)) {
		/* handle array */
		cur_ti = ti_from_relpath_idx(ti_parent, elem);
	} else {
		cur_ti = typeinfo_lookup_follow(ti_parent, elem);
		if (cur_ti == NULL) {
			cur_ti = ti_from_relpath_tryidx(
				ti_parent, elem, s_next);
			s_next = get_path_next(s_next);
		}
	}

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

bool fslnode_by_primpath(const char* path, struct fsl_fuse_node* fn)
{

	struct type_info	*ti_parent;
	char			*child_path;

	ti_parent = ti_from_parentpath(path);
	if (ti_parent == NULL) return false;
	fn->fn_parent = ti_parent;

	child_path = get_path_child(path);
	fn->fn_field = fsl_lookup_field(tt_by_ti(ti_parent), child_path);
	free(child_path);
	if (fn->fn_field == NULL) {
		fn->fn_parent = NULL;
		typeinfo_free(ti_parent);
		return false;
	}
	fn->fn_prim_ti = typeinfo_follow_field(ti_parent, fn->fn_field);

	/* XXX: handle arrays */
	fn->fn_idx = 0;

	return true;
}

bool fslnode_by_arraypath(const char* path, struct fsl_fuse_node* fn)
{
	const struct fsl_rtt_field	*tf;
	struct type_info		*ti_parent;
	char				*child_path;

	ti_parent = ti_from_parentpath(path);
	if (ti_parent == NULL) return false;

	child_path = get_path_child(path);
	tf = fsl_lookup_field(tt_by_ti(ti_parent), child_path);
	free(child_path);
	if (tf == NULL || tf->tf_typenum == TYPENUM_INVALID) {
		typeinfo_free(ti_parent);
		return false;
	}
	if (tf->tf_cond != NULL) {
		if (tf->tf_cond(&ti_clo(ti_parent)) == false) {
			typeinfo_free(ti_parent);
			return false;
		}
	}

	fn->fn_array_field = tf;
	fn->fn_array_parent = ti_parent;

	return true;
}

static bool is_int(const char* s)
{
	for(;*s;s++) { if (*s < '0' || *s > '9') return false; }
	return true;
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
			/* forced array index */
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
		return ret; /* array type */
	}

	if (fslnode_by_arraypath(path, ret) == true) goto done;
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
