#include "runtime.h"
#include <byteswap.h>
#include "debug.h"
#include "io.h"

uint64_t __getLocalArray(
	const struct fsl_rt_closure* clo,
	uint64_t idx, uint64_t bits_in_type,
	uint64_t base_offset, uint64_t bits_in_array)
{
	diskoff_t	real_off, array_off;

	FSL_ASSERT (bits_in_type <= 64);

	array_off = bits_in_type * idx;
	if (array_off > bits_in_array) {
		DEBUG_WRITE("ARRAY OVERFLOW: idx=%"PRIu64
			",bit=%"PRIu64",bia=%"PRIu64"\n",
			idx, bits_in_type, bits_in_array);
		FSL_ASSERT (0 == 1);
	}

	real_off = base_offset + (bits_in_type * idx);
	return __getLocal(clo, real_off, bits_in_type);
}

uint64_t __getLocalPhysMisalign(uint64_t bit_off, uint64_t num_bits)
{
	uint64_t	ret;
	int		bits_left, bits_right;
	int		bytes_full;

	bits_left = 8 - (bit_off % 8);
	if (bits_left > num_bits) bits_left = num_bits;
	bytes_full = (num_bits - bits_left) / 8;
	bits_right = num_bits - (8*bytes_full + bits_left);
	FSL_ASSERT (bits_left > 0 && bits_left < 8);

	/* get the left bits */
	ret = __getLocalPhys(8*(bit_off/8), 8);
	ret >>= (bit_off % 8);

	/* get the full-byte bits */
	if (bytes_full != 0) {
		uint64_t	v;
		v = __getLocalPhys(8*(1+(bit_off/8)), 8*bytes_full);
		ret |= v << bits_left;
	}

	ret &= (1UL << num_bits) - 1;

	/* finally, any slack on the right */
	if (bits_right != 0) {
		uint64_t	v;
		v = __getLocalPhys(8*(1+(bit_off/8)+bytes_full), 8);
		v &= ((1UL << bits_right) - 1);
		ret |= v << (bits_left + 8*bytes_full);
	}

	return ret;
}

uint64_t __swapBytes(uint64_t v, int num_bytes)
{
	switch(num_bytes) {
	case 1: break;
	case 2: v = bswap_16(v); break;
	case 4: v = bswap_32(v); break;
	case 8: v = bswap_64(v);break;
	case 5: {
	uint64_t v_out = 0;
	v_out =   ((v & 0xff00000000) >> (4*8))
		| ((v & 0x00ff000000) >> (2*8))
		| ((v & 0x0000ff0000))
		| ((v & 0x000000ff00) << (2*8))
		| ((v & 0x00000000ff) << (4*8));
	v = v_out;
	break;
	}
	case 6: {
	uint64_t v_out = 0;
	v_out =   ((v & 0xff0000000000) >> (5*8))
		| ((v & 0x00ff00000000) >> (3*8))
		| ((v & 0x0000ff000000) >> (1*8))
		| ((v & 0x000000ff0000) << (1*8))
		| ((v & 0x00000000ff00) << (3*8))
		| ((v & 0x0000000000ff) << 5*8);
	v = v_out;
	break;
	}
	default:
	DEBUG_WRITE ("WHAT: %d\n", num_bytes);
	FSL_ASSERT (0 == 1);
	}

	return v;
}

uint64_t __getLocalPhysMisalignSwp(uint64_t bit_off, uint64_t num_bits)
{
	uint64_t	ret;
	int		bits_left, bits_right;
	int		bytes_full;

	bits_left = 8 - (bit_off % 8);
	if (bits_left > num_bits) bits_left = num_bits;
	bits_right = (bit_off + num_bits) % 8;
	bytes_full = (num_bits - (bits_left+bits_right)) / 8;
	FSL_ASSERT (bits_left > 0 && bits_left < 8);
	FSL_ASSERT (bits_left != 0);
	FSL_ASSERT (bits_left + bits_right + 8*bytes_full == num_bits);

	/* get the left bits */
	ret = __getLocalPhys(8*(bit_off/8), 8) & 0xff;
	ret &= ((1 << bits_left)-1);
	ret <<= num_bits-bits_left;

	/* get the full-byte bits */
	if (bytes_full != 0) {
		uint64_t	v;
		v = __getLocalPhys(8*(1+(bit_off/8)), 8*bytes_full);
		//v = __swapBytes(v, bytes_full);	/* already swapped */
		ret |= v << bits_right;
	}

	/* finally, any slack on the right */
	if (bits_right != 0) {
		uint64_t	v;
		v = __getLocalPhys(8*(1+(bit_off/8)+bytes_full), 8);
		v >>= 8 - bits_right;
		ret |= v;
	}

	ret &= (1UL << num_bits) - 1;

	return ret;
}

void __writeVal(uint64_t loc, uint64_t sz, uint64_t val)
{
	/* assumes *physical address* */
	fsl_wlog_add(&fsl_get_io()->io_wlog, loc, val, sz);
}

void fsl_io_do_wpkt(const struct fsl_rtt_wpkt* wpkt, const uint64_t* params)
{
	int	i;

	FSL_ASSERT (wpkt->wpkt_next == NULL);
	FSL_ASSERT (wpkt->wpkt_blks == NULL);

	DEBUG_IO_ENTER();

#ifdef DEBUG_IO
	for (i = 0; i < wpkt->wpkt_param_c; i++) {
		DEBUG_IO_WRITE("arg[%d] = %"PRIu64, i, params[i]);
	}
#endif

	for (i = 0; i < wpkt->wpkt_func_c; i++) wpkt->wpkt_funcs[i](params);

	for (i = 0; i < wpkt->wpkt_call_c; i++) {
		const struct fsl_rtt_wpkt2wpkt	*w2w = &wpkt->wpkt_calls[i];
		uint64_t	new_params[w2w->w2w_wpkt->wpkt_param_c];
		if (w2w->w2w_cond_f(params) == false) continue;
		w2w->w2w_params_f(params, new_params);
		fsl_io_do_wpkt(w2w->w2w_wpkt, new_params);
	}

	DEBUG_IO_LEAVE();
}

void fsl_io_unhook(struct fsl_rt_io* io, int cb_type)
{
	FSL_ASSERT (cb_type < IO_CB_NUM);
	FSL_ASSERT (io->io_cb[cb_type] != NULL && "Unhooking NULL function");
	io->io_cb[cb_type] = NULL;
}

void fsl_io_steal_wlog(struct fsl_rt_io* io, struct fsl_rt_wlog* dst)
{
	fsl_wlog_copy(dst, &io->io_wlog);
	fsl_wlog_init(&io->io_wlog);
}

void fsl_io_abort_wlog(struct fsl_rt_io* io)
{
	fsl_wlog_init(&io->io_wlog);
}

fsl_io_callback fsl_io_hook(
	struct fsl_rt_io* io, fsl_io_callback cb, int cb_type)
{
	fsl_io_callback	old_cb;

	FSL_ASSERT (cb_type < IO_CB_NUM);
	old_cb = io->io_cb[cb_type];
	io->io_cb[cb_type] = cb;

	return old_cb;
}

uint64_t fsl_io_size(struct fsl_rt_io* io)
{
	return io->io_blksize*io->io_blk_c;
}

void fsl_io_dump_pending(void)
{
	struct fsl_rt_io	*io = fsl_get_io();
	int			i;

	DEBUG_WRITE("Dumping pending");
	for (i = 0; i < io->io_wlog.wl_idx; i++) {
		struct fsl_rt_wlog_ent	*we = &io->io_wlog.wl_write[i];
		uint64_t	cur_v;

		DEBUG_WRITE(
			"PENDING: byteoff=%"PRIu64" (%"PRIu64"); bits=%d. ",
			we->we_bit_addr/8,
			we->we_bit_addr,
			we->we_bits);

		cur_v = __getLocalPhys(we->we_bit_addr, we->we_bits);
		DEBUG_WRITE(
			"v=%"PRIu64" (current=%"PRIu64" / 0x%"PRIx64")",
			we->we_val,
			cur_v, cur_v);
	}
	DEBUG_WRITE("Done dumping pending");
}

void fsl_io_cb_identity(struct fsl_rt_io* io, uint64_t bit_addr) { return; }
