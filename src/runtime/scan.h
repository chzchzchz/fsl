#ifndef SCAN_H
#define SCAN_H

#define SCAN_RET_CONTINUE	0	/* keep on scanning */
#define SCAN_RET_TERMINATE	1	/* do not scan anything else */
#define SCAN_RET_SKIP		2	/* stop scanning this type */
#define SCAN_RET_EXIT_TYPE	3	/* leave scan of parent type */

typedef int(*scan_ti_f)(struct type_info* ti, void* aux);
typedef int(*scan_virt_f)(
	struct type_info* ti,
	const struct fsl_rtt_virt*, int idx, void* aux);
typedef int(*scan_pt_f)(
	struct type_info* ti,
	const struct fsl_rtt_pointsto*, int idx, void* aux);
typedef int(*scan_strong_f)(
	struct type_info* ti, const struct fsl_rtt_field*, void* aux);
typedef int(*scan_cond_f)(
	struct type_info* ti, const struct fsl_rtt_field*, void* aux);
typedef int(*scan_field_f)(
	struct type_info* ti, const struct fsl_rtt_field*, void* aux);

struct scan_ops {
	scan_ti_f		so_ti;
	scan_strong_f		so_strong;
	scan_virt_f		so_virt;
	scan_pt_f		so_pt;
	scan_cond_f		so_cond;
//	scan_ti_field_f		so_field;
};

int scan_type(struct type_info* ti, const struct scan_ops* sf, void* aux);

#endif
