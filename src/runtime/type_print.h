#ifndef TYPE_PRINT_H
#define TYPE_PRINT_H

void typeinfo_print(const struct type_info* ti);
void typeinfo_print_fields(const struct type_info* ti);
void typeinfo_print_path(const struct type_info* cur);
void typeinfo_print_pointsto(const struct type_info* cur);
void typeinfo_print_virt(const struct type_info* cur);
void typeinfo_dump_data(const struct type_info* ti);

#endif
