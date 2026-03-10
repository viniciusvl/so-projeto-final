#include "kernel/module_loader.h"

void run_module(multiboot_module_t *mod)
{
    if(mod == NULL) return;

    typedef void (*call_module_t)(void);

    call_module_t start_program = (call_module_t) mod->mod_start;

    start_program();
}

multiboot_module_t* get_module_list(multiboot_info_t *mbinfo)
{
    if (mbinfo->mods_count == 0) return NULL; // Caso não haja módulos
    
    // Coleta a lista de módulos
    multiboot_module_t *mod = (multiboot_module_t*) mbinfo->mods_addr;

    return mod;
}