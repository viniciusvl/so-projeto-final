#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include "multiboot.h"

void run_module(multiboot_module_t *mod);
multiboot_module_t* get_module_list(multiboot_info_t *mbinfo);

#endif