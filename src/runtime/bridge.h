#ifndef FSLBRIDGE_H
#define FSLBRIDGE_H

enum fsl_node_t
{
	FSL_NODE_TYPE,
	FSL_NODE_PRIMARRAY,
	FSL_NODE_TYPEARRAY,
	FSL_NODE_POINTSTO,	/* lists pointsto elems */
	FSL_NODE_VIRT,		/* lists virt elems */
	FSL_NODE_PRIM		/* raw primitive data */
};

struct fsl_bridge_node
{
	/* user-type */
	struct type_info		*fbn_ti;

	/* fbn */
	struct type_info		*fbn_arr_parent;
	const struct fsl_rtt_field	*fbn_arr_prim_field;
	const struct fsl_rtt_field	*fbn_arr_field;
	const struct fsl_rtt_pointsto	*fbn_arr_pt;
	const struct fsl_rtt_virt	*fbn_arr_vt;
	uint64_t			fbn_arr_max;

	/* primitive */
	struct type_info		*fbn_f_parent;
	const struct fsl_rtt_field	*fbn_f_field;
	struct type_info		*fbn_prim_ti;
};

#define fbn_is_type(x)		(((x)->fbn_ti) != NULL)
#define fbn_is_array(x)		(((x)->fbn_arr_parent) != NULL)
#define fbn_is_array_field(x)	(((x)->fbn_arr_field) != NULL)
#define fbn_is_array_prim(x)	(((x)->fbn_arr_prim_field) != NULL)
#define fbn_is_array_pt(x)	(((x)->fbn_arr_pt) != NULL)
#define fbn_is_array_vt(x)	(((x)->fbn_arr_vt) != NULL)
#define fbn_is_prim(x)		(((x)->fbn_f_parent) != NULL)
#define ti_by_fbn(x) 		((x)->fbn_ti)

uint64_t fbn_bytes(const struct fsl_bridge_node* fbn);
uint64_t fbn_max_idx(const struct fsl_bridge_node* fbn);
enum fsl_node_t fsl_bridge_type(const struct fsl_bridge_node* fbn);
struct fsl_bridge_node* fsl_bridge_idx_into(
		const struct fsl_bridge_node*	fbn,
		unsigned int			idx,
		int*				err);
struct fsl_bridge_node* fsl_bridge_follow_pretty_name(
		struct fsl_bridge_node* fbn,
		const char* pretty_name);
struct fsl_bridge_node* fsl_bridge_follow_name(
		struct type_info* ti_p, /* parentpath */
		const char* child_path);
struct fsl_bridge_node* fsl_bridge_follow_tf(
	struct type_info		*ti_p,
	const struct fsl_rtt_field	*tf);
/* gives ownership of ti to returned node */
struct fsl_bridge_node* fsl_bridge_from_ti(struct type_info* ti);
void fsl_bridge_free(struct fsl_bridge_node* fbn);

#endif
