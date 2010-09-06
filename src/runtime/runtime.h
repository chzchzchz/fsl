#ifndef RUNTIME_H
#define RUNTIME_H


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* XXX TODO Needs local context for multi-threading.. */

#define tt_by_num(x)	(&fsl_rt_table[x])

typedef uint64_t	diskoff_t;
typedef uint64_t	typeoff_t;
typedef uint64_t	typesize_t;
typedef unsigned int	typenum_t;
typedef uint64_t*	parambuf_t;

struct fsl_rt_ctx
{
	unsigned int		fctx_num_types;
	uint64_t		*fctx_type_offsets;
	uint64_t		**fctx_type_params;
	FILE			*fctx_backing;
	struct fsl_rt_virt	*fctx_virt;
};

/* virtual type mapping */
struct fsl_rt_virt
{
	diskoff_t			rtv_off;
	parambuf_t			rtv_params;
	const struct fsl_rt_table_virt	*rtv_f;
	/* XXX needs some smart data structure here */
};

#define TYPENUM_INVALID	(~0)

/* XXX these should take a thunkvar when we support args */
typedef diskoff_t(*thunkf_t)(diskoff_t, parambuf_t );
typedef typesize_t(*sizef_t)(diskoff_t, parambuf_t );
typedef uint64_t(*elemsf_t)(diskoff_t, parambuf_t );
typedef uint64_t(*points_minf_t)(diskoff_t, parambuf_t );
typedef uint64_t(*points_maxf_t)(diskoff_t, parambuf_t );
typedef diskoff_t(*points_rangef_t)(
	diskoff_t, parambuf_t , uint64_t /* idx */, parambuf_t /* out */);
typedef void(*paramsf_t)(diskoff_t, parambuf_t /* in */, parambuf_t /* out */);
typedef bool(*condf_t)(diskoff_t, parambuf_t);
typedef bool(*assertf_t)(diskoff_t, parambuf_t);


struct fsl_rt_thunk_var
{
	diskoff_t	tv_offset;	/* disk offset in bits */
	uint64_t	tv_args[];	/*  args (may be more thunk_vars) */
};

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
};

struct fsl_rt_table_virt
{
	typenum_t	vt_type_src;
	typenum_t	vt_type_virttype;
	points_rangef_t	vt_range;
	points_minf_t	vt_min;
	points_maxf_t	vt_max;
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
	diskoff_t off,
	parambuf_t params,
	uint64_t num_elems);

uint64_t __getLocal(uint64_t bit_off, uint64_t num_bits);
uint64_t __getLocalArray(
	uint64_t idx, uint64_t bits_in_type,
	uint64_t base_offset, uint64_t bits_in_array);
uint64_t __getDynOffset(uint64_t type_num);
void __getDynParams(uint64_t typenum, parambuf_t params_out);
void __setDyn(uint64_t type_num, diskoff_t offset, parambuf_t params);
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
void fsl_rt_uninit(struct fsl_rt_ctx* ctx);
void fsl_rt_dump_dyn(void);
void fsl_virt_set(
	typenum_t src_typenum, /* parameters for parent type instance */
	diskoff_t src_off,
	parambuf_t src_params,
	const struct fsl_rt_table_virt* );
void fsl_virt_clear(void);

/* implemented by tool: */
void tool_entry(void);


#endif
