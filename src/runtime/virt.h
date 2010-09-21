#ifndef RT_VIRT_H
#define RT_VIRT_H

#define fsl_virt_src_types(x)	(((x)->rtm_cached_maxidx-(x)->rtm_cached_minidx)+1)
#define fsl_virt_total_bits(x)	((x)->rtm_cached_srcsz*fsl_virt_src_types(x))

struct fsl_rt_mapping
{
	/* may need reference counting */
	struct fsl_rt_closure* 		rtm_clo; 	/* for eval rtm_virt */
	struct fsl_rt_table_virt* 	rtm_virt;	/* off_v -> off_phys */
	struct fsl_rt_closure*		rtm_dyn; /* saved dyns for rtm_virt */
	uint64_t			rtm_cached_minidx;
	uint64_t			rtm_cached_maxidx;
	uint64_t			rtm_cached_srcsz;
};

struct fsl_rt_mapping*  fsl_virt_alloc(
	struct fsl_rt_closure* parent_clo, struct fsl_rt_table_virt* virt);
uint64_t fsl_virt_xlate(
	const struct fsl_rt_closure* clo, uint64_t bit_off);
diskoff_t fsl_virt_get_nth(const struct fsl_rt_mapping* rtm, unsigned int idx);
void fsl_virt_free(struct fsl_rt_mapping*);

#endif
