//#define DEBUG_SCAN
#include "runtime.h"
#include "debug.h"
#include "type_info.h"
#include "scan.h"
#include "alloc.h"

#define SCAN_FL_DEEP	(1 << 0)

struct scan_stk;

struct scan_stkops {
	int (*sto_init)(struct scan_stk*);
	int (*sto_fini)(struct scan_stk*);
	/* performs op on new element */
	int (*sto_fill_next)(struct scan_stk* src, struct scan_stk* dst);
};

struct scan_stk {
	struct scan_ctx			*stk_ctx;
	const struct scan_stkops	*stk_ops;
	struct scan_stk			*stk_next;

	/* extra data */
	unsigned int			stk_idx;

	/* scan_field_array */
	const struct fsl_rtt_field	*sfa_field;
	uint64_t			sfa_numelems;
	diskoff_t			sfa_off;

	/* scan pt */
	const struct fsl_rtt_pointsto	*spt_pt;
	uint64_t			spt_min, spt_max;

	/* scan vt */
	const struct fsl_rtt_virt	*svt_vt;
};

struct scan_ctx {
	struct type_info		*sctx_ti;
	const struct scan_ops		*sctx_ops;
	void				*sctx_aux;
	uint32_t			sctx_flags;
};

static int stkop_default_fini(struct scan_stk* s) { return SCAN_RET_CONTINUE; }

#define DECL_STKOP(x)						\
static int stkop_##x##_init(struct scan_stk*);				\
static int stkop_##x##_next(struct scan_stk*, struct scan_stk*);	\
const struct scan_stkops scan_stkops_##x = {	\
	.sto_init = stkop_##x##_init,		\
	.sto_fini = stkop_default_fini,		\
	.sto_fill_next = stkop_##x##_next,	\
};

DECL_STKOP(cond);
DECL_STKOP(pointsto);
DECL_STKOP(virt);
DECL_STKOP(fieldstrong);
DECL_STKOP(fieldarray);

#define DECL_STKOP_ALL(x,y)						\
static int stkop_##x##_all_init(struct scan_stk* s) {			\
	s->stk_idx = 0;							\
	return SCAN_RET_CONTINUE;					\
}									\
static int stkop_##x##_all_next(struct scan_stk* s, struct scan_stk* sn)\
{									\
	const struct fsl_rtt_type	*tt = tt_by_ti(s->stk_ctx->sctx_ti); \
	const struct fsl_rtt_##x	*tab = tt->tt_##x;		\
	while (s->stk_idx < tt->tt_##x##_c) {				\
		int err;						\
		s->stk_idx++;						\
		sn->stk_ops = &scan_stkops_##x;				\
		sn->stk_ctx = s->stk_ctx;				\
		sn->y = &tab[s->stk_idx-1];				\
		err = sn->stk_ops->sto_init(sn);			\
		if (!is_ret_done(err)) return err;			\
	}								\
	return SCAN_RET_EXIT_TYPE;					\
}									\
									\
const struct scan_stkops scan_stkops_##x##_all = {	\
	.sto_init = stkop_##x##_all_init,		\
	.sto_fini = stkop_default_fini,			\
	.sto_fill_next = stkop_##x##_all_next,		\
};

DECL_STKOP_ALL(virt, svt_vt);
DECL_STKOP_ALL(pointsto, spt_pt);

static int stkop_type_init(struct scan_stk* stk);
static int stkop_type_fini(struct scan_stk* stk);
static int stkop_type_next(struct scan_stk* stk, struct scan_stk* stk_next);
const struct scan_stkops scan_stkops_type = {
	.sto_init = stkop_type_init,
	.sto_fini = stkop_type_fini,
	.sto_fill_next = stkop_type_next,
};

const struct scan_stkops* scan_stkops_tab[] = {
	&scan_stkops_fieldstrong,
	&scan_stkops_cond,
	&scan_stkops_pointsto_all,
	&scan_stkops_virt_all,
	NULL
};

#define ctx_init(ctx,x,y,z,fl)		\
	do {				\
		(ctx)->sctx_ti = x;	\
		(ctx)->sctx_ops = y;	\
		(ctx)->sctx_aux = z;	\
		(ctx)->sctx_flags = fl;	\
	} while (0)

static int scan_type_fl(
	struct type_info* ti, const struct scan_ops* sops, void* aux,
	uint32_t fl);

static int stkop_type_init(struct scan_stk* stk)
{
	struct scan_ctx	*ctx = stk->stk_ctx;
	if (ctx->sctx_ops->so_ti) {
		int ret = ctx->sctx_ops->so_ti(ctx->sctx_ti, ctx->sctx_aux);
		if (is_ret_done(ret)) return ret;
	}

	stk->stk_idx = 0;
	return SCAN_RET_CONTINUE;
}

static int stkop_type_fini(struct scan_stk* stk)
{
	typeinfo_free(stk->stk_ctx->sctx_ti);
	fsl_free(stk->stk_ctx);
	stk->stk_ctx = NULL;
	return SCAN_RET_CONTINUE;
}

static int create_stk_type(
	const struct scan_ctx* old_ctx,
	struct scan_stk* stk_new,
	struct type_info* new_ti)
{
	int	ret;

	stk_new->stk_ctx = fsl_alloc(sizeof(struct scan_ctx));
	ctx_init(stk_new->stk_ctx, new_ti,
		old_ctx->sctx_ops, old_ctx->sctx_aux, old_ctx->sctx_flags);

	stk_new->stk_ops = &scan_stkops_type;
	ret = stk_new->stk_ops->sto_init(stk_new);
	if (is_ret_done(ret)) {
		fsl_free(stk_new->stk_ctx);
		typeinfo_free(new_ti);
	}
	return ret;
}

static int stkop_type_next(struct scan_stk* stk, struct scan_stk* stk_next)
{
	const struct scan_stkops	*stkops;

	stkops = scan_stkops_tab[stk->stk_idx++];
	if (stkops == NULL) return SCAN_RET_EXIT_TYPE; /* nothing left */

	stk_next->stk_ctx = stk->stk_ctx;
	stk_next->stk_ops = stkops;
	return stkops->sto_init(stk_next);
}

static int stkop_cond_init(struct scan_stk* stk)
{
	struct type_info*	ti = stk->stk_ctx->sctx_ti;
	if (ti_typenum(ti) == TYPENUM_INVALID) return SCAN_RET_EXIT_TYPE;
	stk->stk_idx = 0;
	return SCAN_RET_CONTINUE;
}

static int stkop_cond_next(struct scan_stk* stk, struct scan_stk* stk_next)
{
	struct scan_ctx			*ctx = stk->stk_ctx;
	struct type_info		*ti = ctx->sctx_ti;
	const struct fsl_rtt_field	*tf = NULL;
	const struct fsl_rtt_type	*tt = tt_by_ti(ti);

	for (; stk->stk_idx < tt->tt_fieldtypes_c; stk->stk_idx++) {
		tf = &tt->tt_fieldtypes_thunkoff[stk->stk_idx];
		if (tf->tf_cond != NULL) break;
	}
	/* out of fields? */
	if (stk->stk_idx >= tt->tt_fieldtypes_c) return SCAN_RET_EXIT_TYPE;
	/* bump idx so we will know to handle next */
	stk->stk_idx++;

	/* apply ctx op */
	if (ctx->sctx_ops->so_cond != NULL) {
		int ret = ctx->sctx_ops->so_cond(ti, tf, ctx->sctx_aux);
		if (is_ret_done(ret)) return ret;
	}

	/* verify presence before scanning the thing */
	if (!tf->tf_cond(&ti_clo(ti))) return stkop_cond_next(stk, stk_next);

	stk_next->stk_ops = &scan_stkops_fieldarray;
	stk_next->stk_ctx = stk->stk_ctx;
	stk_next->sfa_field = tf;
	return stk_next->stk_ops->sto_init(stk_next);
}

static int stkop_fieldarray_init(struct scan_stk* stk)
{
	struct type_info		*ti = stk->stk_ctx->sctx_ti;
	const struct fsl_rtt_field	*tf = stk->sfa_field;

	if (tf->tf_typenum == TYPENUM_INVALID) return SCAN_RET_EXIT_TYPE;

	if (	(stk->stk_ctx->sctx_flags & SCAN_FL_DEEP) == 0 &&
		(tf->tf_flags & FIELD_FL_NOFOLLOW) != 0)
		return SCAN_RET_EXIT_TYPE;

	/* init first elem */
	stk->stk_idx = 0;
	stk->sfa_off = tf->tf_fieldbitoff(&ti_clo(ti));
	stk->sfa_numelems = tf->tf_elemcount(&ti_clo(ti));

	return SCAN_RET_CONTINUE;
}

static int stkop_fieldarray_next(struct scan_stk* stk, struct scan_stk* s_next)
{
	const struct fsl_rtt_field	*tf = stk->sfa_field;
	struct type_info		*ti = stk->stk_ctx->sctx_ti;
	struct type_info		*new_ti = NULL;

	for (; stk->stk_idx < stk->sfa_numelems; stk->stk_idx++) {
		new_ti = typeinfo_follow_field_off_idx(
			ti, tf, stk->sfa_off, stk->stk_idx);
		if (new_ti != NULL) break;
		stk->sfa_off += tf->tf_typesize(&ti_clo(ti));
	}

	if (new_ti == NULL) return SCAN_RET_EXIT_TYPE;

	/* move to next element */
	if (stk->stk_idx < stk->sfa_numelems-1) {
		if (ti_typenum(new_ti) == TYPENUM_INVALID)
			stk->sfa_off += tf->tf_typesize(&ti_clo(ti));
		else
			stk->sfa_off += tt_by_ti(new_ti)->tt_size(&ti_clo(new_ti));
	}
	stk->stk_idx++;

	return create_stk_type(stk->stk_ctx, s_next, new_ti);
}

static int stkop_fieldstrong_init(struct scan_stk* stk)
{
	if (ti_typenum(stk->stk_ctx->sctx_ti) == TYPENUM_INVALID)
		return SCAN_RET_EXIT_TYPE;
	stk->stk_idx = 0;
	return SCAN_RET_CONTINUE;
}

static int stkop_fieldstrong_next(struct scan_stk* stk, struct scan_stk* s_n)
{
	struct scan_ctx			*ctx = stk->stk_ctx;
	struct type_info		*ti = ctx->sctx_ti;
	int				ret;
	const struct fsl_rtt_field	*tf;
	const struct fsl_rtt_type	*tt;

	tt = tt_by_ti(ti);
	if (stk->stk_idx >= tt->tt_fieldstrong_c) return SCAN_RET_EXIT_TYPE;
	tf = &tt->tt_fieldstrong[stk->stk_idx++];

	if (ctx->sctx_ops->so_strong != NULL) {
		ret = ctx->sctx_ops->so_strong(ti, tf, ctx->sctx_aux);
		if (is_ret_done(ret)) return ret;
	}

	s_n->stk_ops = &scan_stkops_fieldarray;
	s_n->stk_ctx = stk->stk_ctx;
	s_n->sfa_field = tf;
	ret = s_n->stk_ops->sto_init(s_n);
	if (is_ret_done(ret)) return stkop_fieldstrong_next(stk, s_n);
	return ret;
}

static int stkop_pointsto_init(struct scan_stk* stk)
{
	struct type_info	*ti = stk->stk_ctx->sctx_ti;

	FSL_ASSERT (stk->spt_pt != NULL);
	FSL_ASSERT (stk->spt_pt->pt_iter.it_range != NULL);

	stk->spt_min= stk->spt_pt->pt_iter.it_min(&ti_clo(ti));
	if (stk->spt_min == ~0) return SCAN_RET_EXIT_TYPE;
	stk->spt_max = stk->spt_pt->pt_iter.it_max(&ti_clo(ti));
	if (stk->spt_min > stk->spt_max) return SCAN_RET_EXIT_TYPE;

	stk->stk_idx = stk->spt_min;
	return SCAN_RET_CONTINUE;
}

static int stkop_pointsto_next(struct scan_stk* stk, struct scan_stk* stk_next)
{
	struct scan_ctx			*ctx = stk->stk_ctx;
	struct type_info		*ti = ctx->sctx_ti;
	struct type_info		*new_ti;
	const struct fsl_rtt_type	*tt;

	tt = tt_by_num(stk->spt_pt->pt_iter.it_type_dst);
	if (stk->stk_idx > stk->spt_max) return SCAN_RET_EXIT_TYPE;

	for (; stk->stk_idx <= stk->spt_max; stk->stk_idx++) {
		new_ti = typeinfo_follow_pointsto(ti, stk->spt_pt, stk->stk_idx);
		if (new_ti != NULL) break;
	}

	if (new_ti == NULL) return SCAN_RET_EXIT_TYPE;
	stk->stk_idx++;

	if (ctx->sctx_ops->so_pt != NULL) {
		int ret = ctx->sctx_ops->so_pt(
			ti, stk->spt_pt, stk->stk_idx-1, ctx->sctx_aux);
		if (is_ret_done(ret)) return ret;
	}

	return create_stk_type(stk->stk_ctx, stk_next, new_ti);
}

static int stkop_virt_init(struct scan_stk* stk)
{
	FSL_ASSERT (stk->svt_vt != NULL);
	stk->stk_idx = 0;
	return SCAN_RET_CONTINUE;
}

static int stkop_virt_next(struct scan_stk* stk, struct scan_stk* stk_next)
{
	const struct fsl_rtt_virt	*vt = stk->svt_vt;
	struct scan_ctx			*ctx = stk->stk_ctx;
	struct type_info		*ti_cur;
	int				err_code;

	err_code = TI_ERR_OK;
	while (err_code == TI_ERR_OK || err_code == TI_ERR_BADVERIFY) {
		if (ctx->sctx_ops->so_virt != NULL) {
			int ret = ctx->sctx_ops->so_virt(
				ctx->sctx_ti, vt, stk->stk_idx, ctx->sctx_aux);
			if (is_ret_done(ret)) return ret;
		}

		ti_cur = typeinfo_follow_virt(
			ctx->sctx_ti, vt, stk->stk_idx, &err_code);
		if (ti_cur != NULL) break;
		stk->stk_idx++;
	}

	if (ti_cur == NULL) return SCAN_RET_EXIT_TYPE;

	FSL_ASSERT (ti_cur->ti_td.td_clo.clo_xlate != NULL);

	stk->stk_idx++;
	return create_stk_type(stk->stk_ctx, stk_next, ti_cur);
}

/* free only one stack element */
static void scan_stk_free_ent(struct scan_stk* stk)
{
	stk->stk_ops->sto_fini(stk);
	fsl_free(stk);
}

/* free all elements in stack starting from 'stk' as top */
static void scan_stk_free(struct scan_stk* stk)
{
	while (stk != NULL) {
		struct scan_stk* stk_next = stk->stk_next;
		scan_stk_free_ent(stk);
		stk = stk_next;
	}
}

/* manages pushing / popping states based on ops */
static struct scan_stk* scan_stk_next(
	struct scan_stk* stk,
	struct scan_stk* stk_nextbuf,
	int* err)
{
	if (stk == NULL) return NULL;

	/* add next element, if it exists */
	*err = stk->stk_ops->sto_fill_next(stk, stk_nextbuf);
	if (is_ret_done(*err)) {
		/* no more for this activation, pop parent */
		struct scan_stk	*stk_ret = stk->stk_next;
		scan_stk_free_ent(stk);
		return stk_ret;
	}

	/* push */
	stk_nextbuf->stk_next = stk;
	return stk_nextbuf;
}

static int scan_type_ctx(struct scan_ctx* ctx)
{
	struct scan_stk	*stk_head, *stk_next;
	int		err = SCAN_RET_CONTINUE;

	typeinfo_ref(ctx->sctx_ti);
	stk_head = fsl_alloc(sizeof(struct scan_stk));
	err = create_stk_type(ctx, stk_head, ctx->sctx_ti);
	if (is_ret_done(err)) {
		fsl_free(stk_head);
		return err;
	}

	stk_head->stk_next = NULL;
	stk_next = fsl_alloc(sizeof(struct scan_stk));
	while ((stk_head = scan_stk_next(stk_head, stk_next, &err)) != NULL) {
		/* allocate new buffer if old one was taken */
		if (stk_head == stk_next)
			stk_next = fsl_alloc(sizeof(struct scan_stk));
	}
	fsl_free(stk_next);

	return err;
}

static int scan_type_fl(
	struct type_info* ti, const struct scan_ops* sops, void* aux,
	uint32_t fl)
{
	struct scan_ctx		ctx;

	FSL_ASSERT (sops != NULL);

	if (ti == NULL) return SCAN_RET_CONTINUE;
	if (ti_typenum(ti) == TYPENUM_INVALID) return SCAN_RET_CONTINUE;

	ctx_init(&ctx, ti, sops, aux, fl);
	return scan_type_ctx(&ctx);
}

int scan_type_deep(struct type_info* ti, const struct scan_ops* sops, void* aux)
{
	return scan_type_fl(ti, sops, aux, SCAN_FL_DEEP);
}

int scan_type(struct type_info* ti, const struct scan_ops* sops, void* aux)
{
	return scan_type_fl(ti, sops, aux, 0);
}
