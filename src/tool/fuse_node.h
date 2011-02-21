#ifndef FSL_FUSE_NODE
#define FSL_FUSE_NODE

struct fsl_fuse_node
{
	/* user-type */
	struct type_info		*fn_ti;

	/* fn */
	struct type_info		*fn_arr_parent;
	const struct fsl_rtt_field	*fn_arr_field;
	const struct fsl_rtt_pointsto	*fn_arr_pt;
	const struct fsl_rtt_virt	*fn_arr_vt;

	/* primitive */
	struct type_info		*fn_parent;
	const struct fsl_rtt_field	*fn_field;
	struct type_info		*fn_prim_ti;
};

#define get_fnode(x)	((struct fsl_fuse_node*)(((x))->fh))
#define set_fnode(x,y)	((x))->fh = (uint64_t)((void*)(y))

#define fn_is_type(x)	(((x)->fn_ti) != NULL)
#define fn_is_array(x)	(((x)->fn_arr_parent) != NULL)
#define fn_is_array_field(x) (((x)->fn_arr_field) != NULL)
#define fn_is_array_pt(x) (((x)->fn_arr_pt) != NULL)
#define fn_is_array_vt(x) (((x)->fn_arr_vt) != NULL)
#define fn_is_prim(x)	(((x)->fn_parent) != NULL)

struct fsl_fuse_node* fslnode_by_path(const char* path);
void fslnode_free(struct fsl_fuse_node* fn);

#endif
