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
	struct type_info	*cur_ti, *ret = NULL;
	const char		*s = path, *s_next;
	char			*elem;

	while (*s == '/') s++;
	if (*s == '\0') return ti_parent;
	s_next = strchr(s, '/');
	if (s_next == NULL) s_next = s + strlen(s);

	elem = strndup(s, s_next - s);
	cur_ti = typeinfo_lookup_follow(ti_parent, elem);
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

	/* XXX: handle arrays */
	fn->fn_idx = 0;

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

	if (!fslnode_by_primpath(path, ret)) {
		free(ret);
		return NULL;
	}

	return ret;
}

void fslnode_free(struct fsl_fuse_node* fn)
{
	if (fn->fn_ti != NULL) typeinfo_free(fn->fn_ti);
	if (fn->fn_parent != NULL) typeinfo_free(fn->fn_parent);
	free(fn);
}
