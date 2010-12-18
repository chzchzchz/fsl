#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

typedef struct bitmap{
	uint8_t		*bmp_data;
	unsigned int	bmp_bytes;
	unsigned int	bmp_size;
}bitmap;

#define bmp_num_bits(x)	((x)->bmp_size)

void	bmp_init(bitmap* b, unsigned int elements);
void	bmp_uninit(bitmap* b);

void	bmp_clear(bitmap* b);
int	bmp_set_avail(bitmap* b, unsigned int offset, unsigned int n);
void	bmp_set(bitmap* b, unsigned int offset, unsigned int n);
void	bmp_unset(bitmap* b, unsigned int offset, unsigned int n);
int	bmp_get_set(bitmap* b, unsigned int offset);
int	bmp_is_set(bitmap* b, unsigned int offset);
int	bmp_count_set_contiguous(	bitmap* b, unsigned int offset,
					unsigned int* num);
int	bmp_count_set(bitmap* b, unsigned int offset);
int	bmp_count_avail(bitmap* b, unsigned int offset);
int	bmp_count_contig_set(bitmap* b, unsigned int offset);
int	bmp_count_contig_avail(bitmap* b, unsigned int offset);
#endif
