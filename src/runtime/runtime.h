#ifndef RUNTIME_H
#define RUNTIME_H


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <setjmp.h>

/* XXX TODO Needs local context for multi-threading.. */

#define tt_by_num(x)	(&fsl_rt_table[x])

#define FSL_ENV_VAR_STATFILE	"FSL_ENV_STATFILE"
#define FSL_ENV_VAR_HITFILE	"FSL_ENV_HITFILE"
#define FSL_ENV_VAR_MISSFILE	"FSL_ENV_MISSFILE"

typedef uint64_t	diskoff_t;	/* bits */
typedef uint64_t	voff_t;		/* virtual offset */
typedef uint64_t	poff_t;		/* physical offset */
typedef uint64_t	byteoff_t;
typedef uint64_t	typeoff_t;
typedef uint64_t	typesize_t;
typedef unsigned int	typenum_t;
typedef uint64_t*	parambuf_t;

extern struct fsl_rt_ctx* 	fsl_env;


#define FSL_STAT_ACCESS			0
#define FSL_STAT_PHYSACCESS		1
#define FSL_STAT_BITS_READ		2
#define FSL_STAT_XLATE_CALL		3
#define FSL_STAT_XLATE_ALLOC		4
#define FSL_STAT_COMPUTEARRAYBITS	5
#define FSL_STAT_DYNSET			6
#define FSL_STAT_GETPARAM		7
#define FSL_STAT_GETCLOSURE		8
#define FSL_STAT_GETOFFSET		9
#define FSL_STAT_DYNCOPY		10
#define FSL_STAT_DYNALLOC		11
#define FSL_STAT_TYPEINFO_ALLOC		12
#define FSL_STAT_COMPUTEARRAYBITS_LOOPS 13
#define FSL_NUM_STATS			14
#define FSL_STATS_GET(x,y)		((x)->s_counters[y])
#define FSL_STATS_INC(x,y)		(x)->s_counters[y]++
#define FSL_STATS_ADD(x,y,z)		(x)->s_counters[y] += (z)

struct fsl_rt_stat
{
	uint64_t	s_counters[FSL_NUM_STATS];
};

#include "io.h"

struct fsl_rt_ctx
{
	unsigned int		fctx_num_types;
	struct fsl_rt_closure	*fctx_dyn_closures;
	struct fsl_rt_io	*fctx_io;
	struct fsl_rt_stat	fctx_stat;
	bool			fctx_in_unsafe_op;
	int			fctx_err_unsafe_op;
	jmp_buf			fctx_except;
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
	uint64_t x##_params[tt_by_num(y)->tt_param_c];	\
	NEW_CLO (x,~0,x##_params);			\

#define LOAD_CLO(x,x_tf,y)					\
	do {							\
		(x)->clo_xlate = (y)->clo_xlate;		\
		(x)->clo_offset = (x_tf)->tf_fieldbitoff(y);	\
		(x_tf)->tf_params(y, 0, (x)->clo_params);	\
	} while (0)

#include "dyn.h"
#include "virt.h"

#define TYPENUM_INVALID	(~0)
#define OFFSET_INVALID	((uint64_t)(~0))
#define OFFSET_EOF	((uint64_t)((~0) - 1))
#define offset_is_bad(x)	(((uint64_t)(x)) >= OFFSET_EOF)

/* XXX these should take a thunkvar when we support args */
typedef diskoff_t(*thunkf_t)(const struct fsl_rt_closure*);
typedef typesize_t(*sizef_t)(const struct fsl_rt_closure*);
typedef uint64_t(*elemsf_t)(const struct fsl_rt_closure*);
typedef uint64_t(*points_minf_t)(const struct fsl_rt_closure*);
typedef uint64_t(*points_maxf_t)(const struct fsl_rt_closure*);
typedef diskoff_t(*points_rangef_t)(
	const struct fsl_rt_closure*,
	uint64_t /* idx */,
	parambuf_t /* out */);
typedef void(*paramsf_t)(
	const struct fsl_rt_closure*,
	uint64_t /* array idx within a field */,
	parambuf_t /* out */);
typedef bool(*condf_t)(const struct fsl_rt_closure*);
typedef bool(*assertf_t)(const struct fsl_rt_closure*);

/* if you change a table struct, remember to update it in table_gen.cc!! */
struct fsl_rt_table_type
{
	const char			*tt_name;
	unsigned int			tt_param_c;

	sizef_t				tt_size;

	/* strong user-types */
	unsigned int			tt_fieldstrong_c;
	struct fsl_rt_table_field	*tt_fieldstrong_table;

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

	/* all fields (including union fields) */
	unsigned int			tt_field_c;
	struct fsl_rt_table_field	*tt_field_table;
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

	/* XXX: used for debugging, remove in real life */
	unsigned int	tf_fieldnum;
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
	uint64_t typenum,
	const struct fsl_rt_closure* clo_parent,
	unsigned int fieldall_idx,
	uint64_t num_elems);

void __debugOutcall(uint64_t v);
void __debugClosureOutcall(uint64_t tpenum, struct fsl_rt_closure* clo);
uint64_t __getDynOffset(uint64_t type_num);
void __getDynClosure(uint64_t typenum, struct fsl_rt_closure* clo);
void __getDynParams(uint64_t typenum, parambuf_t params_out);
void *__getDynVirt(uint64_t typenum);
void __setDyn(uint64_t type_num, const struct fsl_rt_closure* clo);

uint64_t __max2(uint64_t a0, uint64_t a1);
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
int tool_entry(int argc, char* argv[]);


#endif
