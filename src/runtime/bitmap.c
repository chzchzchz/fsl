#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "bitmap.h"

static int	bmp_find_avail(bitmap* b, unsigned int offset);
static int	bmp_find_set(bitmap* b, unsigned int offset);

void bmp_init(bitmap* b, unsigned int elements)
{
	b->bmp_bytes = (elements + 7) / 8;
	b->bmp_data = malloc(b->bmp_bytes);
	b->bmp_size = elements;
}

void bmp_clear(bitmap* b)
{
	memset(b->bmp_data, 0, b->bmp_bytes);
}

void bmp_uninit(bitmap* b)
{
	free(b->bmp_data);
}

int bmp_set_avail(bitmap* b, unsigned int offset, unsigned int n)
{
	int	avail_loc;

	assert(offset < b->bmp_size);

	avail_loc = offset;
	do{
		int	count;

		avail_loc = bmp_find_avail(b, avail_loc);
		if(avail_loc == -1)
			return -1;

		count = bmp_count_contig_avail(b, avail_loc);
		if(count < n){
			avail_loc += count;
		}else{
			bmp_set(b, avail_loc, n);
			return avail_loc;
		}
	}while(avail_loc <= (b->bmp_size - n));

	return -1;
}

void bmp_set(bitmap* b, unsigned int offset, unsigned int n)
{
	int	i;
	int	off;
	int	count;

	assert((offset + n) < b->bmp_size);

	count = n;
	off = offset;
	i = 0;

	/* fill bits on left */
	if(off & 7){
		i = 8 - (off & 7);
		if(count < i)
			i = count;

		b->bmp_data[offset / 8] |= ((1 << i) - 1) << (off & 7);

		off += i;
		count -= i;
	}

	/*fill full bytes */
	for(i = 0; i < (count / 8); i++){
		b->bmp_data[i + (off / 8)] = 0xff;
	}

	/* bits on right */
	if(count & 7){
		b->bmp_data[i + (off / 8)] |= (1 << (count & 7)) - 1;
	}
}

void bmp_unset(bitmap* b, unsigned int offset, unsigned int n)
{
	int	i;
	int	off;
	int	count;

	assert((offset + n) < b->bmp_size);

	count = n;
	off = offset;
	i = 0;

	/* fill bits on left */
	if(off & 7){
		uint8_t	mask;

		i = 8 - (off & 7);
		if(count < i)
			i = count;

		mask = ~(((1 << i) - 1) << (off & 7));
		b->bmp_data[offset / 8] &= mask;

		off += i;
		count -= i;
	}

	/*fill full bytes */
	for(i = 0; i < (count / 8); i++){
		b->bmp_data[i + (off / 8)] = 0;
	}

	/* bits on right */
	if(count & 7){
		uint8_t	mask;
		mask = ~((1 << (count & 7)) - 1);
		b->bmp_data[i + (off / 8)] &= mask;
	}
}

int bmp_get_set(bitmap* b, unsigned int offset)
{
	assert(offset < b->bmp_size);

	return bmp_find_set(b, offset);
}

int bmp_count_set_contiguous(bitmap* b, unsigned int offset, unsigned int* num)
{
	int	set_loc;

	assert(offset < b->bmp_size);

	set_loc = bmp_find_set(b, offset);
	if(set_loc == -1){
		*num = 0;
		return -1;
	}

	*num = bmp_count_contig_set(b, set_loc);
	return set_loc;
}

int bmp_is_set(bitmap* b, unsigned int offset)
{
	int	bmp_byte = offset / 8;
	int	bmp_bitoff = offset % 8;
	return (b->bmp_data[bmp_byte] & (1 << bmp_bitoff));
}

int bmp_count_contig_avail(bitmap* b, unsigned int offset)
{
	int	n;
	int	i;

	n = 0;
	for(i = offset / 8; i < b->bmp_bytes; i++){
		int	ii;
		for(ii = offset & 7; ii < 8; ii++){
			if((b->bmp_data[i] & (1 << ii)) == 0)
				n++;
			else
				return n;
		}
		offset = 0;
	}

	return n;
}

static int bmp_find_avail(bitmap* b, unsigned int offset)
{
	int	n;
	int	i;

	n = 0;
	for(i = offset / 8; i < b->bmp_bytes; i++){
		int	ii;
		if(b->bmp_data[i] != 0xff){
			for(ii = offset & 7; ii < 8; ii++){
				if((b->bmp_data[i] & (1 << ii)) == 0)
					return (i * 8 + ii);
			}
		}
		offset = 0;
	}

	return -1;
}

static int bmp_find_set(bitmap* b, unsigned int offset)
{
	int	n;
	int	i;

	n = 0;
	for(i = offset / 8; i < b->bmp_bytes; i++){
		int	ii;

		if(b->bmp_data[i] != 0){
			for(ii = offset & 7; ii < 8; ii++){
				if((b->bmp_data[i] & (1 << ii)) != 0)
					return (i * 8 + ii);
			}
		}
		offset = 0;
	}

	return -1;
}

int bmp_count_contig_set(bitmap* b, unsigned int offset)
{
	int	n;
	int	i;

	n = 0;
	for(i = offset / 8; i < b->bmp_bytes; i++){
		int	ii;
		for(ii = offset & 7; ii < 8; ii++){
			if((b->bmp_data[i] & (1 << ii)) != 0)
				n++;
			else
				return n;
		}
		offset = 0;
	}

	return n;
}

int bmp_count_set(bitmap* b, unsigned int offset)
{
	int	n;
	int	i;

	assert(offset < b->bmp_size);

	n = 0;
	for(i = offset / 8; i < b->bmp_bytes; i++){
		int	ii;
		for(ii = offset & 7; ii < 8; ii++){
			if((b->bmp_data[i] & (1 << ii)) != 0)
				n++;
		}
		offset = 0;
	}

	return n;
}

int bmp_count_avail(bitmap* b, unsigned int offset)
{
	int	n;
	int	i;

	assert(offset < b->bmp_size);

	n = 0;
	for(i = offset / 8; i < b->bmp_bytes; i++){
		int	ii;
		for(ii = offset & 7; ii < 8; ii++){
			if((b->bmp_data[i] & (1 << ii)) == 0)
				n++;
		}
		offset = 0;
	}

	return n;
}
