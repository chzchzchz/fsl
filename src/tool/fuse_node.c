#include <inttypes.h>
#include "fuse_node.h"
#include "type_info.h"
#include "runtime.h"
#include "lookup.h"
#include <string.h>
#include <stdio.h>
#include "alloc.h"
#include "debug.h"

static bool is_int(const char* s);
static struct fsl_bridge_node* fbn_from_rootpath(const char* path);
static struct fsl_bridge_node* fbn_from_relpath(
	struct fsl_bridge_node* fbn_parent, const char* path);


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

static bool is_int(const char* s)
{
	for(;*s;s++) { if (*s < '0' || *s > '9') return false; }
	return true;
}

static struct fsl_bridge_node* fbn_from_rootpath(const char* path)
{
	struct type_info	*ti_origin;
	struct fsl_bridge_node	*ret, *fbn;

	ti_origin = typeinfo_alloc_origin();
	if (ti_origin == NULL) return NULL;
	fbn = fsl_bridge_from_ti(ti_origin);
	if (fbn == NULL) return NULL;

	ret = fbn_from_relpath(fbn, path);
	if (ret != fbn) fsl_bridge_free(fbn);

	return ret;
}

static struct fsl_bridge_node* fbn_from_relpath(
	struct fsl_bridge_node* fbn_parent, const char* path)
{
	const char		*s_next;
	struct fsl_bridge_node	*new_fbn, *ret;
	char			*elem;

	/* no path remains? */
	s_next = get_path_next(path);
	if (s_next == NULL) return fbn_parent;
	elem = get_path_first(path);
	if (elem == NULL) return fbn_parent;

	if (is_int(elem)) {
		/* handle array.. path = /123/abc/def => elem = 123.  */
		int	err;
		new_fbn = fsl_bridge_idx_into(
			fbn_parent, atoi(elem), &err);
	} else {
		new_fbn = fsl_bridge_fbn_follow_name(fbn_parent, elem);
		if (new_fbn == NULL) {
			new_fbn = fsl_bridge_follow_pretty_name(
				fbn_parent, elem);
		}
	}

	free(elem);
	if (new_fbn == NULL) return NULL;

	/* recurse */
	ret = fbn_from_relpath(new_fbn, s_next);
	if (ret != new_fbn) fsl_bridge_free(new_fbn);

	return ret;
}

struct fsl_bridge_node* fslnode_by_path(const char* path)
{
	return fbn_from_rootpath(path);
}
