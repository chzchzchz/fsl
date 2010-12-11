#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "usemap.h"

struct usemap* usemap_alloc(uint64_t um_total_bytes)
{
	struct usemap*	ret;

	ret = malloc(sizeof(struct usemap));
	ret->um_bytes_per_ent = 4*4096;
	ret->um_ents = (um_total_bytes+(4*4096-1))/(4*4096);
	ret->um_map_c = malloc(2*ret->um_ents);
	memset(ret->um_map_c, 0, ret->um_ents*2);

	return ret;
}

void usemap_free(struct usemap* um)
{
	free(um->um_map_c);
	free(um);
}

void usemap_add(struct usemap* um, uint64_t byte_off, uint64_t num_bytes)
{
	um->um_map_c[byte_off/um->um_bytes_per_ent] += num_bytes;
}

void usemap_sub(struct usemap* um, uint64_t byte_off, uint64_t num_bytes)
{
	um->um_map_c[byte_off/um->um_bytes_per_ent] -= num_bytes;
}

int16_t usemap_get_bucket(struct usemap* um, uint64_t byte_off)
{
	assert (byte_off < um->um_total_bytes);
	return um->um_map_c[byte_off/um->um_bytes_per_ent];
}
