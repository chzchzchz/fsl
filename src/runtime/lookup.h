#ifndef FSL_LOOKUP_H
#define FSL_LOOKUP_H

const struct fsl_rtt_field* fsl_lookup_field(
	const struct fsl_rtt_type*, const char* fname);
const struct fsl_rtt_pointsto* fsl_lookup_points(
	const struct fsl_rtt_type* tt, const char* fname);
const struct fsl_rtt_virt* fsl_lookup_virt(
	const struct fsl_rtt_type* tt, const char* fname);
#endif
