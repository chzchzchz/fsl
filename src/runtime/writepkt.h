#ifndef WRITEPKT_H
#define WRITEPKT_H

void wpkt_relocate(
	struct type_info* ti_parent,
	const struct fsl_rtt_reloc* rel,
	struct type_info* rel_sel_ti,
	unsigned int sel_idx,
	unsigned int choice_idx);


#endif
