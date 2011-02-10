#include <string.h>
#include "runtime.h"
#include "lookup.h"

/* XXX: SLOW */
const struct fsl_rtt_field* fsl_lookup_field(
	const struct fsl_rtt_type* tt, const char* fname)
{
	unsigned int	i;
	for (i = 0; i < tt->tt_field_c; i++) {
		const struct fsl_rtt_field*	tf;
		tf = &tt->tt_field_table[i];
		if (strcmp(fname, tf->tf_fieldname) == 0) return tf;
	}
	return NULL;
}

const struct fsl_rtt_pointsto* fsl_lookup_points(
	const struct fsl_rtt_type* tt, const char* fname)
{
	unsigned int	i;

	for (i = 0; i < tt->tt_pointsto_c; i++) {
		const struct fsl_rtt_pointsto*	pt;
		pt = &tt->tt_pointsto[i];
		if (pt->pt_name == NULL) continue;
		if (strcmp(fname, pt->pt_name) == 0) return pt;
	}
	return NULL;
}

const struct fsl_rtt_virt* fsl_lookup_virt(
	const struct fsl_rtt_type* tt, const char* fname)
{
	unsigned int	i;

	for (i = 0; i < tt->tt_virt_c; i++) {
		const struct fsl_rtt_virt*	vt;
		vt = &tt->tt_virt[i];
		if (vt->vt_name == NULL) continue;
		if (strcmp(fname, vt->vt_name) == 0) return vt;
	}
	return NULL;
}
