#include "kernel/module_loader.h"
#include "io/io.h"

void run_module(multiboot_module_t *mod)
{
    if(mod == NULL) return;

    typedef void (*call_module_t)(void);

    call_module_t start_program = (call_module_t) mod->mod_start;

    start_program();
}

/* Retorna o endereço do começo da lista de módulos */
multiboot_module_t* get_module_list(multiboot_info_t *mbinfo)
{

    if (mbinfo->mods_count == 0) return NULL; // Caso não haja módulos
    
    if(mbinfo->mods_count)
    {
        fb_clear();
        char debbug[] = "Existe módulos";
        fb_write(debbug, sizeof(debbug) - 1);
    }

    // Coleta a lista de módulos
    multiboot_module_t *mod = (multiboot_module_t*) mbinfo->mods_addr;

    return mod;
}