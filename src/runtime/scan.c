//#define DEBUG_SCAN
#include <inttypes.h>
#include <assert.h>
#include "runtime.h"
#include "debug.h"
#include "type_info.h"
#include "scan.h"

#define is_ret_done(x)	((x) == SCAN_RET_TERMINATE || (x) == SCAN_RET_EXIT_TYPE)

struct scan_ctx {
	struct type_info		*sctx_ti;
	const struct scan_ops		*sctx_ops;
	void				*sctx_aux;
};

#define ctx_init(ctx,x,y,z)		\
	do {				\
		(ctx)->sctx_ti = x;	\
		(ctx)->sctx_ops = y;	\
		(ctx)->sctx_aux = z;	\
	} while (0)

static int scan_pointsto_all(struct scan_ctx* ctx);
static int scan_virt_all(struct scan_ctx* ctx);
static int scan_strongtype_all(struct scan_ctx* ctx);
static int scan_pointsto(struct scan_ctx* ctx, const struct fsl_rt_table_pointsto*);
static int scan_virt(struct scan_ctx*, const struct fsl_rt_table_virt*);
static int scan_strongtype(struct scan_ctx*, const struct fsl_rt_table_field*);

static int scan_virt_all(struct scan_ctx* ctx)
{
	struct fsl_rt_table_type	*tt;
	unsigned int			i;
	int				ret;

	DEBUG_SCAN_ENTER();
	tt = tt_by_ti(ctx->sctx_ti);
	for (i = 0; i < tt->tt_virt_c; i++) {
		ret = scan_virt(ctx, &tt->tt_virt[i]);
		if (is_ret_done(ret))
			goto done;
	}
	ret = SCAN_RET_CONTINUE;
done:
	DEBUG_SCAN_LEAVE();
	return ret;
}

static int scan_strongtype_all(struct scan_ctx* ctx)
{
	const struct fsl_rt_table_type	*tt;
	unsigned int			i;
	int				ret;

	tt = tt_by_ti(ctx->sctx_ti);
	for (i = 0; i < tt->tt_fieldstrong_c; i++) {
		ret = scan_strongtype(ctx, &tt->tt_fieldstrong_table[i]);
		if (is_ret_done(ret))
			goto done;
	}

	ret = SCAN_RET_CONTINUE;
done:
	return ret;
}

static int scan_pointsto_all(struct scan_ctx* ctx)
{
	const struct fsl_rt_table_type	*tt;
	unsigned int			i;
	int				ret;

	tt = tt_by_ti(ctx->sctx_ti);
	for (i = 0; i < tt->tt_pointsto_c; i++) {
		ret = scan_pointsto(ctx, &tt->tt_pointsto[i]);
		if (is_ret_done(ret))
			goto done;
	}

	ret = SCAN_RET_CONTINUE;
done:
	return ret;
}

static int scan_pointsto(
	struct scan_ctx* ctx, const struct fsl_rt_table_pointsto* pt)
{
	struct fsl_rt_table_type	*tt;
	struct type_info		*ti;
	int				ret;
	uint64_t			k;
	uint64_t			min_idx, max_idx;

	ti = ctx->sctx_ti;

	DEBUG_SCAN_ENTER();
	FSL_DYN_SAVE(dyn_saved);

	assert (pt->pt_range != NULL);

	tt = tt_by_num(pt->pt_type_dst);

	min_idx = pt->pt_min(&ti_clo(ti));
	max_idx = pt->pt_max(&ti_clo(ti));
	if (min_idx > max_idx) {
		FSL_DYN_RESTORE(dyn_saved);
		DEBUG_SCAN_LEAVE();
		return SCAN_RET_CONTINUE;
	}

	DEBUG_SCAN_WRITE(
		"points_to %s %s [%"PRIu64", %"PRIu64"]",
		(pt->pt_name) ? pt->pt_name : "",
		tt->tt_name,
		min_idx, max_idx);

	for (k = min_idx; k <= max_idx; k++) {
		struct type_info	*new_ti;

		DEBUG_SCAN_WRITE("allocating pt[%d]", k);

		FSL_DYN_LOAD(dyn_saved);

		ctx->sctx_ti = ti;
		if (ctx->sctx_ops->so_pt != NULL) {
			ret = ctx->sctx_ops->so_pt(ti, pt, k, ctx->sctx_aux);
			if (is_ret_done(ret))
				goto done;
		}

		new_ti = typeinfo_follow_pointsto(ti, pt, k);
		if (new_ti == NULL) {
			DEBUG_SCAN_WRITE("skipped");
			continue;
		}else
			DEBUG_SCAN_WRITE("found pointsto");

		DEBUG_SCAN_WRITE("Followng points-to");

		ret = scan_type(new_ti, ctx->sctx_ops, ctx->sctx_aux);
		typeinfo_free(new_ti);

		if (is_ret_done(ret))
			goto done;
	}

	ret = SCAN_RET_CONTINUE;
done:
	ctx->sctx_ti = ti;

	FSL_DYN_RESTORE(dyn_saved);
	DEBUG_SCAN_LEAVE();

	return ret;
}

static int scan_virt(struct scan_ctx* ctx, const struct fsl_rt_table_virt* vt)
{
	int				err_code;
	int				ret;
	unsigned int			i;

	DEBUG_SCAN_ENTER();

	FSL_DYN_SAVE(dyn_saved);
	err_code = TI_ERR_OK;
	i = 0;
	while (err_code == TI_ERR_OK || err_code == TI_ERR_BADVERIFY) {
		struct type_info		*ti_cur;

		DEBUG_SCAN_WRITE("alloc virt %s[%d]", vt->vt_name, i);
		FSL_DYN_LOAD(dyn_saved);

		if (ctx->sctx_ops->so_virt) {
			ret = ctx->sctx_ops->so_virt(
				ctx->sctx_ti, vt, i, ctx->sctx_aux);
			if (is_ret_done(ret)) goto done;
		}

		ti_cur = typeinfo_follow_virt(ctx->sctx_ti, vt, i, &err_code);
		if (ti_cur == NULL) {
			DEBUG_SCAN_WRITE(
				"handle_virt: (skipping %s %d err=%d)",
				vt->vt_name, i, err_code);
			i++;
			continue;
		}

		assert (ti_cur->ti_td.td_clo.clo_xlate != NULL);

		DEBUG_SCAN_WRITE("Following virtual %s[%d]", vt->vt_name, i);

		ret = scan_type(ti_cur, ctx->sctx_ops, ctx->sctx_aux);

		DEBUG_SCAN_WRITE("virt done");
		typeinfo_free(ti_cur);
		if (is_ret_done(ret))
			goto done;

		DEBUG_SCAN_WRITE("free virt %s[%d]", vt->vt_name, i);
		i++;
	}

	ret = SCAN_RET_CONTINUE;
done:
	FSL_DYN_RESTORE(dyn_saved);
	DEBUG_SCAN_LEAVE();

	return ret;
}

static int scan_strongtype(
	struct scan_ctx* ctx, const struct fsl_rt_table_field* field)
{
	struct type_info		*ti;
	uint64_t			num_elems;
	unsigned int			i;
	size_t				off;
	int				ret;

	ti = ctx->sctx_ti;
	if (ti_typenum(ti) == TYPENUM_INVALID) return SCAN_RET_CONTINUE;

	if (ctx->sctx_ops->so_strong) {
		ret =  ctx->sctx_ops->so_strong(ti, field, ctx->sctx_aux);
		if (is_ret_done(ret))
			goto done;
	}

	num_elems = field->tf_elemcount(&ti_clo(ti));
	off = field->tf_fieldbitoff(&ti_clo(ti));
	for (i = 0; i < num_elems; i++) {
		struct type_info*		new_ti;

		new_ti = typeinfo_follow_field_off(ti, field, off);
		if (new_ti == NULL) continue;

		/* recurse */
		if (i < num_elems - 1) {
			/* move to next element */
			if (ti_typenum(new_ti) == TYPENUM_INVALID)
				off += field->tf_typesize(&ti_clo(ti));
			else
				off += tt_by_ti(new_ti)->tt_size(&ti_clo(new_ti));
		}

		DEBUG_SCAN_WRITE("following field %s", field->tf_fieldname);

		ret = scan_type(new_ti, ctx->sctx_ops, ctx->sctx_aux);
		typeinfo_free(new_ti);
		if (is_ret_done(ret))
			goto done;
	}

	ret = SCAN_RET_CONTINUE;
done:
	return ret;
}

int scan_type(struct type_info* ti, const struct scan_ops* sops, void* aux)
{
	struct scan_ctx		ctx;
	int			ret;

	assert (sops != NULL);

	if (ti == NULL) return SCAN_RET_CONTINUE;
	if (ti_typenum(ti) == TYPENUM_INVALID) return SCAN_RET_CONTINUE;


	DEBUG_SCAN_ENTER();

	ctx_init(&ctx, ti, sops, aux);

	if (ctx.sctx_ops->so_ti) {
		ret = ctx.sctx_ops->so_ti(ti, aux);
		if (is_ret_done(ret))
			goto done;
	}

	DEBUG_SCAN_WRITE("do strongtypes_all.");
	ret = scan_strongtype_all(&ctx);
	if (is_ret_done(ret)) goto done;

	DEBUG_SCAN_WRITE("do pointsto_all.");
	ret = scan_pointsto_all(&ctx);
	if (is_ret_done(ret)) goto done;

	DEBUG_SCAN_WRITE("do scan virts.");
	ret = scan_virt_all(&ctx);
	if (is_ret_done(ret)) goto done;

	ret = SCAN_RET_CONTINUE;
done:
	DEBUG_SCAN_LEAVE();
	return ret;
}
