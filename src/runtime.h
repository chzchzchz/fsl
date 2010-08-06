#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>
#include <stdio.h>

/* XXX TODO Needs local context for multi-threading.. */


struct fsl_rt_ctx
{
	unsigned int	fctx_num_types;
	uint64_t	*fctx_type_offsets;
	FILE		*fctx_backing;
};


typedef uint64_t	diskoff_t;
typedef uint64_t	typeoff_t;
typedef uint64_t	typesize_t;
typedef unsigned int	typenum_t;
#define TYPENUM_INVALID	(~0)

/* XXX these should take a thunkvar when we support args */
typedef diskoff_t(*thunkf_t)(diskoff_t /* thunk_off_bits */);
typedef typesize_t(*sizef_t)(diskoff_t /* thunk_off_bits */);
typedef uint64_t(*elemsf_t)(diskoff_t /* thunk_off_bits */);

struct fsl_rt_thunk_var 
{
	diskoff_t	tv_offset;	/* disk offset in bits */
	uint64_t	tv_args[];	/*  args (may be more thunk_vars) */
};

/* if you change a table struct, remember to update it in table_gen.cc!! */
struct fsl_rt_table_type
{
	const char			*tt_name;
	sizef_t				tt_size;
	unsigned int			tt_num_fields;
	struct fsl_rt_table_thunk	*tt_field_thunkoff;
};

struct fsl_rt_table_thunk
{
	const char	*tt_fieldname;
	thunkf_t	tt_fieldbitoff;
	typenum_t	tt_typenum;
	elemsf_t	tt_elemcount;
};

struct fsl_rt_table_pointsto
{
	typenum_t	pt_typenum;

};

/* these are in the generated  fsl.table.c */
extern struct fsl_rt_table_type		fsl_rt_table[];
extern unsigned int			fsl_rt_table_entries;
extern unsigned int			fsl_rt_origin_typenum;


/* exposed to llvm */
uint64_t __getLocal(uint64_t bit_off, uint64_t num_bits);
uint64_t __getLocalArray(
	uint64_t idx, uint64_t bits_in_type, 
	uint64_t base_offset, uint64_t bits_in_array);
uint64_t __getDyn(uint64_t type_num);
void __setDyn(uint64_t type_num, uint64_t offset);
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

/* implemented by tool: */
void tool_entry(void);


#endif
