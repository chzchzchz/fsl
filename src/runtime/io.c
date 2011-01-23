#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>
#include "debug.h"
#include "io.h"

uint64_t __getLocalArray(
	const struct fsl_rt_closure* clo,
	uint64_t idx, uint64_t bits_in_type,
	uint64_t base_offset, uint64_t bits_in_array)
{
	diskoff_t	real_off, array_off;

	assert (bits_in_type <= 64);

	array_off = bits_in_type * idx;
	if (array_off > bits_in_array) {
		fprintf(stderr,
			"ARRAY OVERFLOW: idx=%"PRIu64
			",bit=%"PRIu64",bia=%"PRIu64"\n",
			idx, bits_in_type, bits_in_array);
		exit(-5);
	}

	real_off = base_offset + (bits_in_type * idx);
	return __getLocal(clo, real_off, bits_in_type);
}

void __writeVal(uint64_t loc, uint64_t sz, uint64_t val)
{
	/* assumes *physical address* */
	fsl_wlog_add(&fsl_get_io()->io_wlog, loc, val, sz);
}

void fsl_io_do_wpkt(const struct fsl_rtt_wpkt* wpkt, const uint64_t* params)
{
	int	i;

	assert (wpkt->wpkt_next == NULL);
	assert (wpkt->wpkt_blks == NULL);

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
	assert (cb_type < IO_CB_NUM);
	assert (io->io_cb[cb_type] != NULL && "Unhooking NULL function");

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

	assert (cb_type < IO_CB_NUM);
	old_cb = io->io_cb[cb_type];
	io->io_cb[cb_type] = cb;

	return old_cb;
}

ssize_t fsl_io_size(struct fsl_rt_io* io)
{
	return io->io_blksize*io->io_blk_c;
}

void fsl_io_dump_pending(void)
{
	struct fsl_rt_io	*io = fsl_get_io();
	int			i;

	for (i = 0; i < io->io_wlog.wl_idx; i++) {
		struct fsl_rt_wlog_ent	*we = &io->io_wlog.wl_write[i];
		uint64_t	cur_v;

		printf(	"PENDING: byteoff=%"PRIu64" (%"PRIu64"); bits=%d. ",
			we->we_bit_addr/8,
			we->we_bit_addr,
			we->we_bits);

		cur_v = __getLocalPhys(we->we_bit_addr, we->we_bits);
		printf("v=%"PRIu64
			" (current=%"PRIu64" / 0x%"PRIx64")\n",
			we->we_val,
			cur_v, cur_v);
	}
}
