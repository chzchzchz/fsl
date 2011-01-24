#ifndef FSL_FUSE_NODE
#define FSL_FUSE_NODE

struct fsl_fuse_node
{
	/* user-type */
	struct type_info		*fn_ti;

	/* fn */
	struct type_info		*fn_array_parent;
	const struct fsl_rtt_field	*fn_array_field;

	/* primitive */
	struct type_info		*fn_parent;
	const struct fsl_rtt_field	*fn_field;
	int				fn_idx; /* for arrays */
};

#define get_fnode(x)	((struct fsl_fuse_node*)(((x))->fh))
#define set_fnode(x,y)	((x))->fh = (uint64_t)((void*)(y))

#define fn_is_type(x)	(((x)->fn_ti) != NULL)
#define fn_is_array(x)	(((x)->fn_array_parent) != NULL)
#define fn_is_prim(x)	(((x)->fn_parent) != NULL)

struct fsl_fuse_node* fslnode_by_path(const char* path);
void fslnode_free(struct fsl_fuse_node* fn);

#endif
