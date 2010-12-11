#ifndef USEMAP_H
#define USEMAP_H

#include <stdint.h>

struct usemap{
	uint64_t	um_total_bytes;
	unsigned int	um_bytes_per_ent;
	unsigned int	um_ents;
	int16_t		*um_map_c;
};

#define usemap_bucket_size(x)	((x)->um_bytes_per_ent

struct usemap* usemap_alloc(uint64_t um_total_bytes);
void usemap_free(struct usemap* um);
void usemap_add(struct usemap* um, uint64_t byte_off, uint64_t num_bytes);
void usemap_sub(struct usemap* um, uint64_t byte_off, uint64_t num_bytes);
int16_t usemap_get_bucket(struct usemap* um, uint64_t byte_off);

#endif