/* main runtime file */
//#define DEBUG_RT
//#define DEBUG_IO
#include "debug.h"
#include "io.h"
#include "alloc.h"
#include "runtime.h"

extern uint64_t fsl_num_types;

struct fsl_rt_ctx* 		fsl_env;
static int			cur_debug_write_val = 0;
const char* fsl_stat_fields[FSL_NUM_STATS] =
{
	"getLocals",		/* FSL_STAT_ACCESS */
	"getLocalsPhy", 	/* FSL_STAT_PHYSACCESS */
	"br",			/* FSL_STAT_BITS_READ */
	"xlate_call",		/* FSL_STAT_XLATE_CALL */
	"xlate_alloc",		/* FSL_STAT_XLATE_ALLOC */
	"computeArrayBits",	/* FSL_STAT_COMPUTEARRAYBITS */
	"typeinfo_alloc",	/* FSL_STAT_TYPEINFO_ALLOC */
	"comparray_elems",	/* FSL_STAT_COMPUTEARRAYBITS_LOOPS */
	"writes",		/* FSL_STAT_WRITES */
	"bw",			/* FSL_STAT_BITS_WRITTEN */
	"xlate_hit",		/* FSL_STAT_XLATE_HIT */
	"iocache_miss",		/* FSL_STAT_IOCACHE_MISS */
	"iocache_hit",		/* FSL_STAT_IOCACHE_HIT */
	"iocache_drop"		/* FSL_STAT_IOCACHE_DROP */
};

void __debugOutcall(uint64_t v)
{
	DEBUG_WRITE("DEBUG_WRITE: %"PRIu64"\n", v);
	cur_debug_write_val = v;
}

void __debugClosureOutcall(uint64_t typenum, struct fsl_rt_closure* clo)
{
	FSL_ASSERT (clo != NULL);

	DEBUG_WRITE("DEBUG_WRITE_TYPE: %s-- voff=%"PRIu64". poff=%"PRIu64". xlate=%p\n",
		tt_by_num(typenum)->tt_name,
		clo->clo_offset,
		(clo->clo_xlate) ?
			fsl_virt_xlate(clo, clo->clo_offset) : clo->clo_offset,
		clo->clo_xlate);
}

uint64_t fsl_fail(uint64_t x)
{
	DEBUG_WRITE("Failed for some reason...\n");
	FSL_ASSERT (0 == 1);
	return ~0;
}

/* compute the number of bits in a given array */
/* TODO: need way to check to see if uses an index value-- if not, we
 * don't need to reload the params all the time */
typesize_t __computeArrayBits(
	uint64_t typenum_parent,
	const struct fsl_rt_closure* clo_parent,
	unsigned int field_idx,
	uint64_t num_elems)
{
	const struct fsl_rtt_type	*parent_tt, *elem_tt;
	const struct fsl_rtt_field	*tf;
	typenum_t			elem_type;
	unsigned int			i;
	typesize_t			total_bits;

	DEBUG_RT_ENTER();

	FSL_ASSERT (typenum_parent < fsl_rtt_entries && "Bad parent type");

	parent_tt = tt_by_num(typenum_parent);
	FSL_ASSERT (field_idx < parent_tt->tt_field_c && "Fieldnum overflow");

	tf = &parent_tt->tt_field_table[field_idx];
	if (field_idx != tf->tf_fieldnum)
		DEBUG_WRITE("field_idx = %d, tf->tf_fieldnum=%d, tf->tf_fieldname=%s\n",
			field_idx,
			tf->tf_fieldnum, tf->tf_fieldname);
	FSL_ASSERT (field_idx == tf->tf_fieldnum && "Fieldnum mismatch");

	elem_type = tf->tf_typenum;
	FSL_ASSERT (elem_type < fsl_rtt_entries && "Bad array type");
	elem_tt = tt_by_num(elem_type);

	NEW_EMPTY_CLO			(cur_clo, elem_type);

	/* save old dyn closure value */
	DEBUG_RT_WRITE("Get initial closure for type '%s'", elem_tt->tt_name);

	/* get base closure */
	LOAD_CLO(&cur_clo, tf, clo_parent);

	total_bits = 0;
	DEBUG_RT_WRITE("Looping over %d elems of type %s", num_elems, elem_tt->tt_name);
	for (i = 0; i < num_elems; i++) {
		typesize_t		cur_size;

		cur_size = elem_tt->tt_size(&cur_clo);

		total_bits += cur_size;
		cur_clo.clo_offset += cur_size;

		/* don't overflow */
		if (i != (num_elems - 1))
			tf->tf_params(clo_parent, i+1, cur_clo.clo_params);
	}
	DEBUG_RT_WRITE("Looping finished. sz=%"PRIu64, total_bits);


	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_COMPUTEARRAYBITS);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_COMPUTEARRAYBITS_LOOPS, num_elems);

	DEBUG_RT_LEAVE();

	return total_bits;
}

int fsl_rt_uninit(struct fsl_rt_ctx* fctx)
{

	FSL_ASSERT (fctx != NULL);

	if (fctx->fctx_exit_f != NULL) {
		int	ret;
		ret = fctx->fctx_exit_f();
		if (ret != 0) return ret;
	}

	fsl_io_free(fctx->fctx_io);
	fsl_free(fctx);

	return 0;
}

void fsl_vars_from_env(struct fsl_rt_ctx* fctx)
{
	FSL_ASSERT (fctx != NULL);
	FSL_ASSERT (fctx->fctx_io != NULL && "Is the file open?");

	__FROM_OS_BDEV_BYTES = fsl_io_size(fctx->fctx_io);
	__FROM_OS_BDEV_BLOCK_BYTES = fctx->fctx_io->io_blksize;
	__FROM_OS_SB_BLOCKSIZE_BYTES = fctx->fctx_io->io_blksize;
}

/* TODO: track accesses for write filter */
static void fsl_do_memo(const struct fsl_memo_t* m)
{
	struct fsl_rt_closure	*clo;

	if (m->m_typenum == TYPENUM_INVALID) {
		/* primitive */
		__fsl_memotab[m->m_tabidx] = m->m_f.f_prim();
		return;
	}

	clo = (struct fsl_rt_closure*)(&__fsl_memotab[m->m_tabidx]);
	clo->clo_params = (parambuf_t)(clo+1);	/* params immediately follow */
	m->m_f.f_type(clo);
}

void fsl_load_memo(void)
{
	unsigned int	i;
	FSL_ASSERT (fsl_env != NULL);
	for (i = 0; i < __fsl_memof_c; i++)
		fsl_do_memo(&__fsl_memotab_funcs[i]);
}

uint64_t __getLocal(
	const struct fsl_rt_closure* clo, uint64_t bit_off, uint64_t num_bits)
{
	uint64_t		ret;

	FSL_ASSERT (num_bits <= 64);
	FSL_ASSERT (num_bits > 0);

	FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_ACCESS);
	FSL_STATS_ADD(&fsl_env->fctx_stat, FSL_STAT_BITS_READ, num_bits);

	DEBUG_IO_ENTER();

	DEBUG_IO_WRITE("Requesting IO: bitoff=%"PRIu64". clo_off=%"PRIu64,
		bit_off, clo->clo_offset);

	if (clo->clo_xlate != NULL) {
		/* xlate path */
		uint64_t	bit_off_old, bit_off_next, bit_off_last;

		bit_off_old = bit_off;
		DEBUG_IO_WRITE("BIT_COUNT=%"PRIu64, num_bits);
		DEBUG_IO_WRITE("BIT_OFF_OLD=%"PRIu64, bit_off_old);
		bit_off = fsl_virt_xlate(clo, bit_off_old);

		/* ensure read will go to only a contiguous range (e.g. xlate
		 * not sliced too thin) */
		bit_off_next = bit_off_old + 8*((num_bits - 1)/8);
		DEBUG_IO_WRITE("BIT_OFF_NEXT=%"PRIu64, bit_off_next);
		bit_off_last = fsl_virt_xlate(clo, bit_off_next);
		FSL_ASSERT ((bit_off + 8*((num_bits-1)/8)) == (bit_off_last) &&
			"Discontiguous getLocal not permitted");
	}

	ret = __getLocalPhys(bit_off, num_bits);
	DEBUG_IO_WRITE(
		"Returning IO: bitoff = %"PRIu64" // bits=%"PRIu64" // v = %"PRIu64,
			bit_off, num_bits, ret);

	DEBUG_IO_LEAVE();

	if (num_bits == 1) FSL_ASSERT (ret < 2 && "BADMASK.");

	return ret;
}

bool fsl_is_int(const char* s)
{
	while (*s) if (*s < '0' || *s > '9') return false; else s++;
	return true;
}
