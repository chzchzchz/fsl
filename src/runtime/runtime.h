#ifndef RUNTIME_H
#define RUNTIME_H


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* XXX TODO Needs local context for multi-threading.. */

#define tt_by_num(x)	(&fsl_rt_table[x])

#define FSL_ENV_VAR_STATFILE	"FSL_ENV_STATFILE"

typedef uint64_t	diskoff_t;
typedef uint64_t	typeoff_t;
typedef uint64_t	typesize_t;
typedef unsigned int	typenum_t;
typedef uint64_t*	parambuf_t;

struct fsl_rt_stat
{
	unsigned int	s_access_c;
	uint64_t	s_bits_read;
	uint64_t	s_xlate_call_c;
	uint64_t	s_xlate_alloc_c;
	uint64_t	s_comp_array_bits_c;
	uint64_t	s_dyn_set_c;
	uint64_t	s_get_param_c;
	uint64_t	s_get_closure_c;
	uint64_t	s_get_offset_c;
};

struct fsl_rt_ctx
{
	unsigned int		fctx_num_types;
	struct fsl_rt_closure	*fctx_dyn_closures;
	FILE			*fctx_backing;
	struct fsl_rt_stat	fctx_stat;
};

struct fsl_rt_mapping;

struct fsl_rt_closure
{
	diskoff_t			clo_offset; /* virtual offset of type */
	parambuf_t			clo_params; /* params into type */
	struct fsl_rt_mapping		*clo_xlate; /* "page table"/address space */

};

#define NEW_CLO(x,y,z)	NEW_VCLO(x,y,z,NULL)

#define NEW_VCLO(x,y,z,t)			\
	struct fsl_rt_closure x;		\
	x.clo_offset = y;			\
	x.clo_params = z;			\
	x.clo_xlate = t;
#define NEW_EMPTY_CLO(x,y)				\
	struct fsl_rt_closure	x;			\
	uint64_t x##_params[tt_by_num(y)->tt_param_c];

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

#define TYPENUM_INVALID	(~0)

/* XXX these should take a thunkvar when we support args */
typedef diskoff_t(*thunkf_t)(const struct fsl_rt_closure*);
typedef typesize_t(*sizef_t)(const struct fsl_rt_closure*);
typedef uint64_t(*elemsf_t)(const struct fsl_rt_closure*);
typedef uint64_t(*points_minf_t)(const struct fsl_rt_closure*);
typedef uint64_t(*points_maxf_t)(const struct fsl_rt_closure*);
typedef diskoff_t(*points_rangef_t)(
	const struct fsl_rt_closure*, uint64_t /* idx */, parambuf_t /* out */);
typedef void(*paramsf_t)(const struct fsl_rt_closure*, parambuf_t /* out */);
typedef bool(*condf_t)(const struct fsl_rt_closure*);
typedef bool(*assertf_t)(const struct fsl_rt_closure*);

/* if you change a table struct, remember to update it in table_gen.cc!! */
struct fsl_rt_table_type
{
	const char			*tt_name;
	unsigned int			tt_param_c;

	sizef_t				tt_size;

	/* strong user-types */
	unsigned int			tt_field_c;
	struct fsl_rt_table_field	*tt_field_thunkoff;

	unsigned int			tt_pointsto_c;
	struct fsl_rt_table_pointsto	*tt_pointsto;

	unsigned int			tt_assert_c;
	struct fsl_rt_table_assert	*tt_assert;

	unsigned int			tt_virt_c;
	struct fsl_rt_table_virt	*tt_virt;

	/* all non-union fields */
	unsigned int			tt_fieldall_c;
	struct fsl_rt_table_field	*tt_fieldall_thunkoff;

	/* all types that are not strictly weak */
	unsigned int			tt_fieldtypes_c;
	struct fsl_rt_table_field	*tt_fieldtypes_thunkoff;

};

struct fsl_rt_table_field
{
	const char	*tf_fieldname;
	typenum_t	tf_typenum;
	/* size does not change in field (useful for arrays.) */
	bool		tf_constsize;

	thunkf_t	tf_fieldbitoff;
	elemsf_t	tf_elemcount;
	sizef_t		tf_typesize;
	condf_t		tf_cond;
	paramsf_t	tf_params;
};

struct fsl_rt_table_pointsto
{
	typenum_t	pt_type_dst;
	points_rangef_t	pt_range;
	points_minf_t	pt_min;
	points_maxf_t	pt_max;
	const char*	pt_name;
};

struct fsl_rt_table_virt
{
	typenum_t	vt_type_src;		/* ret type for range */
	typenum_t	vt_type_virttype;	/* type we convert to */
	points_rangef_t	vt_range;
	points_minf_t	vt_min;
	points_maxf_t	vt_max;
	const char*	vt_name;
};

struct fsl_rt_table_assert
{
	assertf_t	as_assertf;
};


/* exported variables from types module.. */
extern uint64_t __FROM_OS_BDEV_BYTES;
extern uint64_t __FROM_OS_BDEV_BLOCK_BYTES;
extern uint64_t __FROM_OS_SB_BLOCKSIZE_BYTES;

/* these are in the generated  fsl.table.c */
extern struct fsl_rt_table_type		fsl_rt_table[];
extern unsigned int			fsl_rt_table_entries;
extern unsigned int			fsl_rt_origin_typenum;
extern char				fsl_rt_fsname[];

#define FSL_DEBUG_FL_GETLOCAL		0x1
extern int				fsl_rt_debug;

/* exposed to llvm */
typesize_t __computeArrayBits(
	uint64_t elem_type,
	struct fsl_rt_closure* clo,
	uint64_t num_elems);

uint64_t __getLocal(
	const struct fsl_rt_closure*, uint64_t bit_off, uint64_t num_bits);
uint64_t __getLocalArray(
	const struct fsl_rt_closure*,
	uint64_t idx, uint64_t bits_in_type,
	uint64_t base_offset, uint64_t bits_in_array);

uint64_t __getDynOffset(uint64_t type_num);
void __debugOutcall(uint64_t v);
void __getDynClosure(uint64_t typenum, struct fsl_rt_closure* clo);
void __getDynParams(uint64_t typenum, parambuf_t params_out);
void __setDyn(uint64_t type_num, const struct fsl_rt_closure* clo);


uint64_t __max2(
	uint64_t a0, uint64_t a1);
uint64_t __max3(uint64_t a0, uint64_t a1, uint64_t a2);
uint64_t __max4(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3);
uint64_t __max5(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
uint64_t __max6(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
		uint64_t a5);
uint64_t __max7(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
		uint64_t a5, uint64_t a6);
uint64_t fsl_fail(void);

/* not exposed to llvm */
struct fsl_rt_ctx* fsl_rt_init(const char* fsl_rt);
struct fsl_rt_closure* fsl_rt_dyn_swap(struct fsl_rt_closure* dyns);
void fsl_rt_uninit(struct fsl_rt_ctx* ctx);

void fsl_dyn_dump(void);
struct fsl_rt_closure* fsl_dyn_alloc(void);
void fsl_dyn_free(struct fsl_rt_closure*);
struct fsl_rt_closure* fsl_dyn_copy(const struct fsl_rt_closure* src);

/* virt functions */
struct fsl_rt_mapping*  fsl_virt_alloc(
	struct fsl_rt_closure* parent_clo, struct fsl_rt_table_virt* virt);
uint64_t fsl_virt_xlate(
	const struct fsl_rt_closure* clo, uint64_t bit_off);
void fsl_virt_free(struct fsl_rt_mapping*);

/* implemented by tool: */
int tool_entry(int argc, char* argv[]);


#endif
