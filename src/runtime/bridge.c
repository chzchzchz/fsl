#include "type_info.h"
#include "alloc.h"
#include "debug.h"
#include "lookup.h"
#include "bridge.h"

static struct fsl_bridge_node *fbn_alloc(void)
{
	struct fsl_bridge_node	*ret;
	ret = fsl_alloc(sizeof(*ret));
	if (ret != NULL) memset(ret, 0, sizeof(*ret));
	return ret;
}

struct fsl_bridge_node* fsl_bridge_follow_tf(
	struct type_info		*ti_p,
	const struct fsl_rtt_field	*tf)
{
	struct fsl_bridge_node	*fbn;
	uint64_t		elems;

	if (tf->tf_cond != NULL)
		if (tf->tf_cond(&ti_clo(ti_p)) == false)
			return NULL;

	elems = tf->tf_elemcount(&ti_clo(ti_p));
	if (elems == 0) return NULL;

	fbn = fbn_alloc();
	if (elems == 1) {
		struct type_info	*ti;
		ti = typeinfo_follow_field(ti_p, tf);
		if (tf->tf_typenum == TYPENUM_INVALID) {
			fbn->fbn_f_parent = typeinfo_ref(ti_p);
			fbn->fbn_f_field  = tf;
			fbn->fbn_prim_ti = ti;
		} else
			fbn->fbn_ti = ti;
	} else {
		/* array type */
		fbn->fbn_arr_parent = typeinfo_ref(ti_p);
		fbn->fbn_arr_max = elems;
		if (tf->tf_typenum == TYPENUM_INVALID)
			fbn->fbn_arr_prim_field = tf;
		else
			fbn->fbn_arr_field = tf;
	}

	return fbn;
}

struct fsl_bridge_node* fsl_bridge_follow_name(
	struct type_info* ti_p, /* parent*/
	const char* child_path /* name */)
{
	struct fsl_bridge_node		*fbn;
	const struct fsl_rtt_field	*tf;
	const struct fsl_rtt_pointsto	*pt;
	const struct fsl_rtt_virt	*vt;

	if ((tf = fsl_lookup_field(tt_by_ti(ti_p), child_path)))
		return fsl_bridge_follow_tf(ti_p, tf);

	fbn = fbn_alloc();
	fbn->fbn_arr_max = ~0;
	if ((pt = fsl_lookup_points(tt_by_ti(ti_p), child_path))) {
		uint64_t	pt_min, pt_max;
		pt_min = pt->pt_iter.it_min(&ti_clo(ti_p));
		if (pt_min == ~0) goto fail;
		pt_max = pt->pt_iter.it_max(&ti_clo(ti_p));
		if (pt_min > pt_max) goto fail;
		fbn->fbn_arr_pt = pt;
		fbn->fbn_arr_max = (pt_max-pt_min)+1;
	} else if ((vt = fsl_lookup_virt(tt_by_ti(ti_p), child_path))) {
		fbn->fbn_arr_vt = vt;
	} else
		goto fail;

	fbn->fbn_arr_parent = typeinfo_ref(ti_p);
	return fbn;
fail:
	fsl_free(fbn);
	return NULL;
}

struct fsl_bridge_node* fsl_bridge_idx_into(
	const struct fsl_bridge_node*	fbn,
	unsigned int			idx,
	int*				err)
{
	struct fsl_bridge_node	*new_fbn;
	struct type_info	*ret_ti;

	FSL_ASSERT (fbn_is_array(fbn));

	if (idx >= fbn->fbn_arr_max) {
		*err = TI_ERR_BADIDX;
		return NULL;
	}

	*err = 0;
	new_fbn = fbn_alloc();
	if (fbn->fbn_arr_field) {
		ret_ti = typeinfo_follow_field_idx(
			fbn->fbn_arr_parent, fbn->fbn_arr_field, idx);
	} else if (fbn->fbn_arr_pt) {
		ret_ti = typeinfo_follow_pointsto(
			fbn->fbn_arr_parent, fbn->fbn_arr_pt, idx);
	} else if (fbn->fbn_arr_vt) {
		ret_ti = typeinfo_follow_virt(
			fbn->fbn_arr_parent,
			fbn->fbn_arr_vt,
			idx,
			err);
	} else {
		FSL_ASSERT (0 == 1 && "BOGUS BRIDGE VAL");
		ret_ti = NULL;
	}

	new_fbn->fbn_ti = ret_ti;	/* XXX SUPPORT PRIMS */
	return new_fbn;
}

void fsl_bridge_free(struct fsl_bridge_node* fbn)
{
	if (fbn->fbn_ti != NULL) typeinfo_free(fbn->fbn_ti);
	if (fbn->fbn_f_parent != NULL) typeinfo_free(fbn->fbn_f_parent);
	if (fbn->fbn_arr_parent != NULL) typeinfo_free(fbn->fbn_arr_parent);
	if (fbn->fbn_prim_ti != NULL) typeinfo_free(fbn->fbn_prim_ti);
	fsl_free(fbn);
}

struct fsl_bridge_node* fsl_bridge_from_ti(struct type_info* ti)
{
	struct fsl_bridge_node	*fbn;
	FSL_ASSERT(ti_typenum(ti) != TYPENUM_INVALID);
	fbn = fbn_alloc();
	fbn->fbn_ti = ti;	/* passes ownership, don't ref */
	return fbn;
}

enum fsl_node_t fsl_bridge_type(const struct fsl_bridge_node* fbn)
{
	if (fbn_is_type(fbn)) return FSL_NODE_TYPE;
	if (fbn_is_prim(fbn)) return FSL_NODE_PRIM;
	if (fbn_is_array_field(fbn)) return FSL_NODE_TYPEARRAY;
	if (fbn_is_array_pt(fbn)) return FSL_NODE_POINTSTO;
	if (fbn_is_array_vt(fbn)) return FSL_NODE_VIRT;
	if (fbn_is_array_prim(fbn)) return FSL_NODE_PRIMARRAY;
	FSL_ASSERT (0 == 1 && "UNKNOWN BRIDGE TYPE");
	return FSL_NODE_TYPE;
}

uint64_t fbn_bytes(const struct fsl_bridge_node* fbn)
{
	uint64_t	num_elems;
	if (fbn->fbn_ti) return ti_size(fbn->fbn_ti)/8;
	if (fbn->fbn_prim_ti) return ti_size(fbn->fbn_prim_ti)/8;
	if (fbn_is_array_field(fbn)) {
		return ti_field_size(
			fbn->fbn_arr_parent,
			fbn->fbn_arr_field,
			&num_elems)/8;
	}
	if (fbn_is_array_prim(fbn)) {
		return ti_field_size(
			fbn->fbn_arr_parent,
			fbn->fbn_arr_prim_field,
			&num_elems)/8;
	}

	return 0;
}
