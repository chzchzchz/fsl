#include <linux/buffer_head.h>

#include "debug.h"
#include "runtime.h"
#include "cache.h"
#include "io.h"
#include "iopriv.h"

static struct fsl_io_cache_ent* fsl_io_cache_evict(
	struct fsl_io_cache* ioc, uint64_t cache_line);
static const uint8_t* fsl_io_cache_put(
	struct fsl_rt_io* io, uint64_t cache_line);
const uint8_t *fsl_io_cache_hitmiss(struct fsl_rt_io* io, uint64_t line_begin);
static const uint8_t* fsl_io_cache_find(
	struct fsl_io_cache* ioc, uint64_t cache_line);

void fsl_io_cache_init(struct fsl_io_cache* ioc)
{
	unsigned int	i;
	FSL_ASSERT (ioc != NULL);
	memset(ioc, 0, sizeof(struct fsl_io_cache));
	for (i = 0; i < FSL_IO_CACHE_ENTS; i++)
		ioc->ioc_ents[i].ce_addr = ~0;
}

static const uint8_t* fsl_io_cache_find(
	struct fsl_io_cache* ioc, uint64_t cache_line)
{
	struct fsl_io_cache_ent	*ce;

	ce = &ioc->ioc_ents[cache_line % FSL_IO_CACHE_ENTS];
	if (ce->ce_addr == cache_line) {
		ioc->ioc_hits++;
		return ce->ce_data;
	}

	ioc->ioc_misses++;
	return NULL;
}

static struct fsl_io_cache_ent* fsl_io_cache_evict(
	struct fsl_io_cache* ioc, uint64_t cache_line)
{
	struct fsl_io_cache_ent	*to_evict;

	//to_evict = &ioc->ioc_ents[ioc->ioc_misses % FSL_IO_CACHE_ENTS];
	to_evict = &ioc->ioc_ents[cache_line % FSL_IO_CACHE_ENTS];
	/* Don't clear-- assume will be used immediately */

	return to_evict;
}

static const uint8_t* fsl_io_cache_put(struct fsl_rt_io* io, uint64_t cache_line)
{
	struct fsl_io_cache_ent		*ce;
	off_t				file_offset;
	struct fsl_rt_io_priv		*iop = io->io_priv;
	struct buffer_head		*bh;
	int				err;

	/* get fresh line */
	ce = fsl_io_cache_evict(&iop->iop_cache, cache_line);
	ce->ce_addr = cache_line;
	FSL_ASSERT (ce != NULL);

	file_offset = cache_line * FSL_IO_CACHE_BYTES;

	FSL_ASSERT(io_bdev(io) != NULL);
	bh = __getblk(io_bdev(io),  io_boff_to_blk(io, file_offset), PAGE_SIZE);
	FSL_ASSERT (bh != NULL);
	lock_buffer(bh);
	err = bh_submit_read(bh);
	unlock_buffer(bh);
	FSL_ASSERT (err == 0 && "ERROR READING");
	memcpy(	ce->ce_data,
		bh->b_data + io_boff_to_blkoff(io, file_offset),
		FSL_IO_CACHE_BYTES);
	__brelse(bh);

	return ce->ce_data;
}

uint64_t fsl_io_cache_get_unaligned(
	struct fsl_rt_io* io, uint64_t bit_off, int num_bits)
{
	const uint8_t		*cache_line[2];
	uint8_t			join_buf[8];
	uint64_t		ret;
	uint64_t		line_begin, line_end;
	unsigned int		cache0_off;
	unsigned int		cache0_len, cache1_len, buf_len;
	int			i;

	DEBUG_IO_ENTER();

	line_begin = bit_off / FSL_IO_CACHE_BITS;
	line_end = ((bit_off + num_bits - 1) / FSL_IO_CACHE_BITS);
	FSL_ASSERT ((line_begin+1) == line_end && "Cache lines too small?");

	cache_line[0] = fsl_io_cache_hitmiss(io, bit_off);
	cache_line[1] = fsl_io_cache_hitmiss(io, bit_off + FSL_IO_CACHE_BITS);

	FSL_ASSERT ((bit_off % 8) == 0);

	cache0_off = (bit_off / 8) - (line_begin*FSL_IO_CACHE_BYTES);
	cache0_len = FSL_IO_CACHE_BYTES - cache0_off;
	cache1_len = ((num_bits+7)/8) - cache0_len;
	memcpy(join_buf, &cache_line[0][cache0_off], cache0_len);
	memcpy(&join_buf[cache0_len], cache_line[1], cache1_len);
	buf_len = cache0_len + cache1_len;
	FSL_ASSERT (buf_len <= 8);
	FSL_ASSERT (((num_bits / 8)-1) < 8);

	// copy bits, byte by byte
	// XXX need to change to support arbitrary endians (this is little)
	ret = 0;
	for (i = (num_bits / 8) - 1; i >= 0; i--) {
		ret <<= 8;
		ret += join_buf[i];
	}

	DEBUG_IO_LEAVE();

	return ret;

}

const uint8_t *fsl_io_cache_hitmiss(struct fsl_rt_io* io, uint64_t bit_off)
{
	const uint8_t	*cache_line;
	uint64_t	line_begin;

	line_begin = bit_off / FSL_IO_CACHE_BITS;
	cache_line = fsl_io_cache_find(&io->io_priv->iop_cache, line_begin);
	if (cache_line == NULL) {
		DEBUG_IO_WRITE("Missed line: %lld", line_begin);
		FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_IOCACHE_MISS);
		cache_line = fsl_io_cache_put(io, line_begin);
		if (io->io_cb_miss != NULL) io->io_cb_miss(io, bit_off);
	} else {
		DEBUG_IO_WRITE("Got line: %lld", line_begin);
		FSL_STATS_INC(&fsl_env->fctx_stat, FSL_STAT_IOCACHE_HIT);
		if (io->io_cb_hit != NULL) io->io_cb_hit(io, bit_off);
	}

	FSL_ASSERT (cache_line != NULL);

	return cache_line;
}

uint64_t fsl_io_cache_get(struct fsl_rt_io* io, uint64_t bit_off, int num_bits)
{
	const uint8_t		*cache_line;
	uint64_t		ret = 0;
	uint64_t		line_begin, line_end;

	DEBUG_IO_ENTER();

	line_begin = bit_to_line(bit_off);
	line_end = bit_to_line(bit_off + num_bits - 1);

	DEBUG_IO_WRITE(
		"LINE: %lld--%lld byteoff=(%lld--%lld)",
		line_begin, line_end, bit_off/8, (bit_off+num_bits)/8);
	if (line_begin != line_end)
		return fsl_io_cache_get_unaligned(io, bit_off, num_bits);

	cache_line = fsl_io_cache_hitmiss(io, bit_off);
	cache_line += (bit_off/8) % FSL_IO_CACHE_BYTES;

	FSL_ASSERT ((bit_off % 8) == 0 && "NOT BYTE ALIGNED??");

	// copy bits
	// XXX need to change to support arbitrary endians (this is little)
	switch((num_bits+7)/8) {
	case 0:
	case 1:
		ret = cache_line[0];
		break;
	case 2:
		ret = ((const uint16_t*)cache_line)[0];
		break;
	case 3:
		ret = ((((const uint32_t*)cache_line)[0]) & 0xffffff);
		break;
	case 4:
		ret = ((const uint32_t*)cache_line)[0];
		break;
	case 5:
		ret = ((const uint64_t*)cache_line)[0] & 0xffffffffff;
		break;
	case 6:
		ret = ((const uint64_t*)cache_line)[0]  & 0xfffffffffffff;
		break;
	case 7:
		ret = ((const uint64_t*)cache_line)[0]  & 0xfffffffffffffff;
		break;
	case 8:
		ret = ((const uint64_t*)cache_line)[0];
		break;
	default:
		FSL_ASSERT (0 == 1);
	}
	ret >>= bit_off % 8;
	if (num_bits < 64) ret &= (1LL << num_bits)-1;
	DEBUG_IO_WRITE("cached_val = %lld", ret);

	DEBUG_IO_LEAVE();

	return ret;
}

void fsl_io_cache_drop_bytes(
	struct fsl_rt_io* io,
	uint64_t byte_off, uint64_t num_bytes)
{
	struct fsl_io_cache	*ioc;
	uint64_t		first_line, last_line;
	uint64_t		cur_line;

	ioc = &io->io_priv->iop_cache;
	first_line = byte_to_line(byte_off);
	last_line = byte_to_line(byte_off+num_bytes+FSL_IO_CACHE_BYTES-1);

	for (cur_line = first_line; cur_line <= last_line; cur_line++) {
		uint64_t	cur_addr;
		uint64_t	cache_idx;

		cache_idx = cur_line % FSL_IO_CACHE_ENTS;
		cur_addr = ioc->ioc_ents[cache_idx].ce_addr;
		if (cur_addr == cur_line) {
			ioc->ioc_ents[cache_idx].ce_addr = ~0;
		}
	}
}
