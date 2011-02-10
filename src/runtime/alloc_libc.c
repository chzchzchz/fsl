#include <stdlib.h>
#include "alloc.h"

void* fsl_alloc(unsigned int sz) { return malloc(sz); }
void fsl_free(void* ptr) { free(ptr); }
