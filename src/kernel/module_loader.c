#include "kernel/module_loader.h"
#include "io/io.h"

#define MODULE_LOADER_MAX_MODULES 8
#define MODULE_CMDLINE_MAX_LEN    63

struct loaded_module {
    uint32_t mod_start;
    uint32_t mod_end;
    char cmdline[MODULE_CMDLINE_MAX_LEN + 1];
};

static struct loaded_module loaded_modules[MODULE_LOADER_MAX_MODULES];
static uint32_t loaded_modules_count;

static void copy_cmdline(char *dst, const char *src)
{
    uint32_t i;

    if (src == 0) {
        dst[0] = '\0';
        return;
    }

    for (i = 0; i < MODULE_CMDLINE_MAX_LEN && src[i] != '\0'; i++)
        dst[i] = src[i];

    dst[i] = '\0';
}

static int path_matches_cmdline(const char *path, const char *cmdline)
{
    uint32_t i = 0;

    while (path[i] != '\0' && cmdline[i] != '\0' && cmdline[i] != ' ') {
        if (path[i] != cmdline[i])
            return 0;
        i++;
    }

    if (path[i] != '\0')
        return 0;

    return (cmdline[i] == '\0' || cmdline[i] == ' ');
}

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

void module_loader_set_modules(multiboot_module_t *mods, uint32_t count)
{
    if (mods == 0 || count == 0) {
        loaded_modules_count = 0;
        return;
    }

    if (count > MODULE_LOADER_MAX_MODULES)
        count = MODULE_LOADER_MAX_MODULES;

    loaded_modules_count = count;

    for (uint32_t i = 0; i < count; i++) {
        loaded_modules[i].mod_start = mods[i].mod_start;
        loaded_modules[i].mod_end   = mods[i].mod_end;
        copy_cmdline(loaded_modules[i].cmdline, (const char *)mods[i].cmdline);
    }
}

int module_loader_find_module_by_path(const char *path, uint32_t *mod_start, uint32_t *mod_end)
{
    if (path == 0 || mod_start == 0 || mod_end == 0)
        return -1;

    for (uint32_t i = 0; i < loaded_modules_count; i++) {
        if (path_matches_cmdline(path, loaded_modules[i].cmdline)) {
            *mod_start = loaded_modules[i].mod_start;
            *mod_end   = loaded_modules[i].mod_end;
            return 0;
        }
    }

    return -1;
}