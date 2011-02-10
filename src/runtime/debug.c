#include <assert.h>
#include "debug.h"

void fsl_debug_return_to_state(statenum_t sn)
{
	statenum_t	old_sn;
	assert (sn <= fsl_debug_depth && "Can't jump forward in debug state");

	old_sn = fsl_debug_depth;
	fsl_debug_depth = sn;
	fsl_debug_write("debug: Skipped back %d levels.", sn - old_sn);
}