#include <linux/slab.h>
#include "alloc.h"

void* fsl_alloc(unsigned int sz) { return kmalloc(sz, GFP_KERNEL); }
void fsl_free(void* ptr) { kfree(ptr); }
