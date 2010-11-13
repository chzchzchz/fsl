#ifndef TYPE_PRINT_H
#define TYPE_PRINT_H

void typeinfo_print(const struct type_info* ti);
void typeinfo_print_fields(const struct type_info* ti);
void typeinfo_print_pointstos(const struct type_info* cur);
void typeinfo_print_virts(const struct type_info* cur);
void typeinfo_print_path(const struct type_info* cur);
void typeinfo_dump_data(const struct type_info* ti);

void typeinfo_print_value(const struct type_info* ti);

void typeinfo_print_field_value(
	const struct type_info		*ti,
	const struct fsl_rt_table_field	*field);

#endif
