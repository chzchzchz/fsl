#include <inttypes.h>
#include "fuse_node.h"
#include "type_info.h"
#include "runtime.h"
#include "lookup.h"
#include <string.h>
#include <stdio.h>
#include "alloc.h"
#include "debug.h"

static struct type_info* ti_from_rootpath(const char* path);
static struct type_info* ti_from_relpath(
	struct type_info* ti_parent, const char* path);
static struct type_info* ti_from_parentpath(const char* path);
static char* get_path_child(const char* path);
static bool is_int(const char* s);

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

static bool is_int(const char* s)
{
	for(;*s;s++) { if (*s < '0' || *s > '9') return false; }
	return true;
}

static struct type_info* ti_from_relpath_tryidx(
	struct type_info* ti_p, const char* elem, const char* s_next)
{
	struct type_info	*ret_ti = NULL;
	char			*next_elem;

	next_elem = get_path_first(s_next);
	if (next_elem == NULL) goto done;

	if (!is_int(next_elem))
		ret_ti = typeinfo_follow_into_name(ti_p, elem, next_elem);
	else
		ret_ti = typeinfo_lookup_follow_idx(ti_p, elem, atoi(next_elem));

	free(next_elem);
done:
	return ret_ti;
}

/* gets a ti given a current parent (ti_parent) and the remaining path */
static struct type_info* ti_from_relpath(
	struct type_info* ti_parent, const char* path)
{
	struct type_info	*cur_ti = NULL, *ret = NULL;
	const char		*s_next;
	char			*elem;

	/* no path remains? */
	s_next = get_path_next(path);
	if (s_next == NULL) return ti_parent;
	elem = get_path_first(path);
	if (elem == NULL) return ti_parent;

	if (is_int(elem)) {
		/* handle array.. path = /123/abc/def => elem = 123.  */
		cur_ti = typeinfo_reindex(ti_parent, atoi(elem));
	} else {
		/* handle scalar */
		cur_ti = typeinfo_lookup_follow(ti_parent, elem);
		if (cur_ti == NULL) {
			/* might be array.. find first index that works */
			cur_ti = ti_from_relpath_tryidx(
				ti_parent, elem, s_next);
			s_next = get_path_next(s_next);
		}
	}

	free(elem);
	if (cur_ti == NULL) return NULL;

	/* recurse */
	ret = ti_from_relpath(cur_ti, s_next);
	if (ret != cur_ti) typeinfo_free(cur_ti);

	return ret;
}

static struct type_info* ti_from_rootpath(const char* path)
{
	struct type_info	*ti_origin, *ret;

	ti_origin = typeinfo_alloc_origin();
	if (ti_origin == NULL) return NULL;
	ret = ti_from_relpath(ti_origin, path);
	if (ret != ti_origin) typeinfo_free(ti_origin);

	return ret;
}

/* path=/abc/def/ghi/ => ret=/abc/def */
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

/* handle array type base */
static bool fslnode_by_arraypath(
	struct fsl_fuse_node* fn,
	struct type_info* ti_p, /* parentpath */
	const char* child_path)
{
	const struct fsl_rtt_field	*tf;
	const struct fsl_rtt_pointsto	*pt;
	const struct fsl_rtt_virt	*vt;

	if ((tf = fsl_lookup_field(tt_by_ti(ti_p), child_path))) {
		if (tf->tf_cond != NULL)
			if (tf->tf_cond(&ti_clo(ti_p)) == false)
				return false;
		if (tf->tf_typenum == TYPENUM_INVALID) {
			fn->fn_parent = ti_p;
			fn->fn_field  = tf;
			fn->fn_prim_ti = typeinfo_follow_field(
				ti_p, fn->fn_field);
			return true;
		}
		fn->fn_arr_field = tf;
	} else if ((pt = fsl_lookup_points(tt_by_ti(ti_p), child_path))) {
		fn->fn_arr_pt = pt;
	} else if ((vt = fsl_lookup_virt(tt_by_ti(ti_p), child_path))) {
		fn->fn_arr_vt = vt;
	} else
		return false;

	fn->fn_arr_parent = ti_p;
	return true;
}

static void fslnode_root_loaded(
	const char* path_child, struct fsl_fuse_node* ret)
{
	struct type_info		*ti_parent;
	const struct fsl_rtt_field	*tf;

	if (path_child != NULL) {
		/* forced array index */
		bool	is_elem_num;
		is_elem_num = is_int(path_child);
		if (is_elem_num) return;
		if (ti_from_name(ret->fn_ti)) return;
		path_child = NULL;
	}
	FSL_ASSERT (path_child == NULL);

	ti_parent = ret->fn_ti->ti_prev;
	if (ti_parent == NULL) return;
	if ((tf = ret->fn_ti->ti_field) != NULL) {
		if (tf->tf_elemcount(&ti_clo(ti_parent)) == 1) return;
		ret->fn_arr_field = tf;
	} else if (ret->fn_ti->ti_points != NULL) {
		ret->fn_arr_pt = ret->fn_ti->ti_points;
	} else if (ret->fn_ti->ti_virt != NULL) {
		ret->fn_arr_vt = ret->fn_ti->ti_virt;
	} else
		return;

	ret->fn_arr_parent = ti_parent;
	typeinfo_ref(ti_parent);
	typeinfo_free(ret->fn_ti);
	ret->fn_ti = NULL;
}

struct fsl_fuse_node* fslnode_by_path(const char* path)
{
	struct fsl_fuse_node	*ret;
	struct type_info	*ti_parent;
	char			*p_child;

	ret = fsl_alloc(sizeof(*ret));
	if (ret == NULL) return NULL;

	memset(ret, 0, sizeof(*ret));
	ret->fn_ti = ti_from_rootpath(path);
	p_child = get_path_child(path);
	if (ret->fn_ti != NULL) {
		fslnode_root_loaded(p_child, ret);
		goto done;
	}

	if (p_child == NULL) goto err;

	ti_parent = ti_from_parentpath(path);
	if (ti_parent == NULL) goto err;

	if (fslnode_by_arraypath(ret, ti_parent, p_child) == true) goto done;

	typeinfo_free(ti_parent);
err:
	if (p_child != NULL) free(p_child);
	fsl_free(ret);
	return NULL;

done:
	if (p_child != NULL) free(p_child);
	return ret;
}

void fslnode_free(struct fsl_fuse_node* fn)
{
	if (fn->fn_ti != NULL) typeinfo_free(fn->fn_ti);
	if (fn->fn_parent != NULL) typeinfo_free(fn->fn_parent);
	if (fn->fn_arr_parent != NULL) typeinfo_free(fn->fn_arr_parent);
	if (fn->fn_prim_ti != NULL) typeinfo_free(fn->fn_prim_ti);
	fsl_free(fn);
}
