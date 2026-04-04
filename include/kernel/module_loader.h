#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include <stdint.h>
#include "multiboot.h"

void run_module(multiboot_module_t *mod);
multiboot_module_t* get_module_list(multiboot_info_t *mbinfo);
void module_loader_set_modules(multiboot_module_t *mods, uint32_t count);
int module_loader_find_module_by_path(const char *path, uint32_t *mod_start, uint32_t *mod_end);

#endif