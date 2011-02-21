#ifndef RUNTIME_H
#define RUNTIME_H

#ifndef __KERNEL__
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#define FSL_RAND()	rand()
#define FSL_ATOI(x)	atoi(x)
#else
#include <stdbool.h>
extern uint32_t random32(void);
extern long simple_strtol(const char*, char**, unsigned int);
#define FSL_RAND()	random32()
#define FSL_ATOI(x)	simple_strtol(x, NULL, 10)
#endif

/* XXX TODO Needs local context for multi-threading.. */

struct fsl_rt_io;
typedef void (*fsl_io_callback)(struct fsl_rt_io*, uint64_t /* bit off */);

#define tt_by_num(x)	(&fsl_rt_table[x])

#define FSL_ENV_VAR_STATFILE	"FSL_ENV_STATFILE"
#define FSL_ENV_VAR_HITFILE	"FSL_ENV_HITFILE"
#define FSL_ENV_VAR_MISSFILE	"FSL_ENV_MISSFILE"
#define FSL_ENV_VAR_WRITEFILE	"FSL_ENV_WRITEFILE"

typedef uint64_t	diskoff_t;	/* bits */
typedef uint64_t	voff_t;		/* virtual offset */
typedef uint64_t	poff_t;		/* physical offset */
typedef uint64_t	byteoff_t;
typedef uint64_t	bitoff_t;
typedef uint64_t	typeoff_t;
typedef uint64_t	typesize_t;
typedef uint32_t	typenum_t;
typedef uint64_t*	parambuf_t;

typedef uint64_t(*memof_t)(void);
extern uint64_t			__fsl_memotab[];
extern int			__fsl_memotab_sz;
extern memof_t			__fsl_memotab_funcs[];
extern int			__fsl_mode;

extern struct fsl_rt_ctx* 	fsl_env;
#define fsl_err_reset()		do { fsl_env->fctx_failed_assert = 0; } while (0)
#define fsl_err_get()		fsl_env->fctx_failed_assert

#define FSL_MODE_BIGENDIAN		1
#define FSL_MODE_LITTLEENDIAN		0

#define FSL_STAT_ACCESS			0
#define FSL_STAT_PHYSACCESS		1
#define FSL_STAT_BITS_READ		2
#define FSL_STAT_XLATE_CALL		3
#define FSL_STAT_XLATE_ALLOC		4
#define FSL_STAT_COMPUTEARRAYBITS	5
#define FSL_STAT_TYPEINFO_ALLOC		6
#define FSL_STAT_COMPUTEARRAYBITS_LOOPS 7
#define FSL_STAT_WRITES			8
#define FSL_STAT_BITS_WRITTEN		9
#define FSL_STAT_XLATE_HIT		10
#define FSL_STAT_IOCACHE_MISS		11
#define FSL_STAT_IOCACHE_HIT		12
#define FSL_NUM_STATS			13
#define FSL_STATS_GET(x,y)		((x)->s_counters[y])
#define FSL_STATS_INC(x,y)		(x)->s_counters[y]++
#define FSL_STATS_ADD(x,y,z)		(x)->s_counters[y] += (z)

struct fsl_rt_stat { uint64_t	s_counters[FSL_NUM_STATS]; };

struct fsl_rt_except
{
	bool			ex_in_unsafe_op;
	int			ex_err_unsafe_op;
	struct type_info	*ex_caller;
};

struct fsl_rt_ctx
{
	unsigned int		fctx_num_types;
	struct fsl_rt_io	*fctx_io;
	struct fsl_rt_stat	fctx_stat;
	struct fsl_rt_except	fctx_except;
	const char*		fctx_failed_assert;
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

#include "virt.h"

#define TYPENUM_INVALID	(~0)
#define OFFSET_INVALID	((uint64_t)(~0))
#define OFFSET_EOF	((uint64_t)((~0) - 1))
#define offset_is_bad(x)	(((uint64_t)(x)) >= OFFSET_EOF)
#define offset_in_range(x)	(((uint64_t)(x)) < (__FROM_OS_BDEV_BYTES*8))

typedef diskoff_t(*thunkf_t)(const struct fsl_rt_closure*);
typedef typesize_t(*sizef_t)(const struct fsl_rt_closure*);
typedef uint64_t(*elemsf_t)(const struct fsl_rt_closure*);
typedef void(*paramsf_t)(
	const struct fsl_rt_closure*,
	uint64_t /* array idx within a field */,
	parambuf_t /* out */);
typedef uint64_t(*points_minf_t)(const struct fsl_rt_closure*);
typedef uint64_t(*points_maxf_t)(const struct fsl_rt_closure*);
typedef diskoff_t(*points_rangef_t)(
	const struct fsl_rt_closure*,
	uint64_t /* idx */,
	parambuf_t /* out */,
	void** /* virt_mapping */);
typedef bool(*condf_t)(const struct fsl_rt_closure*);
typedef bool(*condidxf_t)(const struct fsl_rt_closure*, uint64_t);
typedef bool(*assertf_t)(const struct fsl_rt_closure*);
typedef void(*wpktf_t)(const uint64_t* params);
typedef void(*wpkt2wpktf_t)(const uint64_t* params_in, uint64_t* params_out);
typedef bool(*wpkt2wpktcond_t)(const uint64_t* params_in);
typedef void(*wpkt_paramf_t)(
	const struct fsl_rt_closure*,
	uint64_t* params_in, /* input (e.g. indices, etc) */
	uint64_t* params_out /* output */);

/* if you change a table struct, remember to update it in table_gen.cc!! */
struct fsl_rtt_type
{
	const char			*tt_name;
	unsigned int			tt_param_c;
	unsigned int			tt_arg_c;

	sizef_t				tt_size;

	/* strong user-types */
	unsigned int			tt_fieldstrong_c;
	const struct fsl_rtt_field	*tt_fieldstrong_table;

	unsigned int			tt_pointsto_c;
	const struct fsl_rtt_pointsto	*tt_pointsto;

	unsigned int			tt_assert_c;
	const struct fsl_rtt_assert	*tt_assert;

	unsigned int			tt_virt_c;
	const struct fsl_rtt_virt	*tt_virt;

	unsigned int			tt_reloc_c;
	const struct fsl_rtt_reloc	*tt_reloc;

	/* all non-union fields */
	unsigned int			tt_fieldall_c;
	const struct fsl_rtt_field	*tt_fieldall_thunkoff;

	/* all types that are not strictly weak */
	unsigned int			tt_fieldtypes_c;
	const struct fsl_rtt_field	*tt_fieldtypes_thunkoff;

	/* all fields (including union fields) */
	unsigned int			tt_field_c;
	const struct fsl_rtt_field	*tt_field_table;
};

#define FIELD_FL_CONSTSIZE	0x1 /* size same for all elems in arrays */
#define FIELD_FL_FIXED		0x2
#define FIELD_FL_NOFOLLOW	0x4

struct fsl_rt_iter
{
	typenum_t	it_type_dst;
	points_rangef_t	it_range;
	points_minf_t	it_min;
	points_maxf_t	it_max;
};

struct fsl_rtt_field
{
	const char		*tf_fieldname;
	typenum_t		tf_typenum;
	uint32_t		tf_flags;

	thunkf_t		tf_fieldbitoff;
	elemsf_t		tf_elemcount;
	sizef_t			tf_typesize;
	condf_t			tf_cond;
	paramsf_t		tf_params;

//	struct fsl_rt_iter	tf_iter;

	/* XXX: used for debugging, remove in real life */
	unsigned int	tf_fieldnum;
};

struct fsl_rtt_pointsto
{
	struct fsl_rt_iter	pt_iter;
	const char*		pt_name;
};

struct fsl_rtt_virt
{
	typenum_t		vt_type_virttype;	/* type we convert to */
	struct fsl_rt_iter	vt_iter;
	const char*		vt_name;
};

struct fsl_rtt_assert
{
	assertf_t	as_assertf;
	const char	*as_name;
};

struct fsl_rtt_wpkt
{
	unsigned int			wpkt_param_c;	/* number of arg elems */

	unsigned int			wpkt_func_c;
	wpktf_t				*wpkt_funcs;

	unsigned int			wpkt_call_c;
	struct fsl_rtt_wpkt2wpkt	*wpkt_calls;	/* calls to wpkts */

	unsigned int			wpkt_blk_c;
	struct fsl_rtt_wpkt		*wpkt_blks;
	struct fsl_rtt_wpkt		*wpkt_next;
};

struct fsl_rtt_wpkt2wpkt
{
	wpkt2wpktf_t			w2w_params_f;
	wpkt2wpktcond_t			w2w_cond_f;
	const struct fsl_rtt_wpkt	*w2w_wpkt;
};

struct fsl_rtt_wpkt_inst
{
	wpkt_paramf_t			wpi_params;
	const struct fsl_rtt_wpkt	*wpi_wpkt;
};

struct fsl_rtt_reloc
{
	struct fsl_rt_iter		rel_sel;
	struct fsl_rt_iter		rel_choice;
	condidxf_t			rel_ccond;

	const struct fsl_rtt_wpkt_inst	rel_alloc;
	const struct fsl_rtt_wpkt_inst	rel_replace;
	const struct fsl_rtt_wpkt_inst	rel_relink;

	const char			*rel_name;
};

/* exported variables from types module.. */
extern uint64_t __FROM_OS_BDEV_BYTES;
extern uint64_t __FROM_OS_BDEV_BLOCK_BYTES;
extern uint64_t __FROM_OS_SB_BLOCKSIZE_BYTES;

/* these are in the generated  fsl.table.c */
extern const struct fsl_rtt_type	fsl_rt_table[];
extern unsigned int			fsl_rtt_entries;
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
uint64_t __toPhys(const struct fsl_rt_closure* clo, uint64_t off);

uint64_t __max2(uint64_t a0, uint64_t a1);
uint64_t __max3(uint64_t a0, uint64_t a1, uint64_t a2);
uint64_t __max4(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3);
uint64_t __max5(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4);
uint64_t __max6(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
		uint64_t a5);
uint64_t __max7(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
		uint64_t a5, uint64_t a6);
uint64_t fsl_fail(uint64_t);

/* not exposed to llvm */
void fsl_rt_uninit(struct fsl_rt_ctx* ctx);
void fsl_load_memo(void);
void fsl_vars_from_env(struct fsl_rt_ctx* fctx);

/* implemented by tool: */
int tool_entry(int argc, char* argv[]);

#endif
