#include "syscall/syscall.h"
#include "io/serial_ports.h"
#include "kernel/module_loader.h"
#include "process/process.h"
#include "scheduler/scheduler.h"
#include "memory/paging.h"
#include "memory/pfa.h"
#include "memory/kheap.h"

#define EXEC_PATH_MAX            64
#define EXEC_COPY_CHUNK          128
#define USER_TEXT_BASE           0x00000000
#define USER_STACK_PAGE_VADDR    0xBFFFF000
#define USER_STACK_TOP           0xC0000000
#define USER_CODE_SELECTOR_R3    (0x18 | 0x3)
#define USER_DATA_SELECTOR_R3    (0x20 | 0x3)

extern uint32_t page_directory[];
extern uint32_t kernel_stack_top;

static void destroy_user_space(uint32_t pdt_phys);

static uint32_t min_u32(uint32_t a, uint32_t b)
{
    return (a < b) ? a : b;
}

static int copy_user_string(const char *user_ptr, char *kernel_buffer, uint32_t max_len)
{
    if (user_ptr == 0 || kernel_buffer == 0 || max_len == 0)
        return -1;

    for (uint32_t i = 0; i < max_len; i++) {
        char c = user_ptr[i];
        kernel_buffer[i] = c;

        if (c == '\0')
            return 0;
    }

    kernel_buffer[max_len - 1] = '\0';
    return 0;
}

static uint32_t alloc_zeroed_frame(void)
{
    uint32_t frame = pfa_alloc();

    if (frame == 0)
        return 0;

    uint8_t *mapped = (uint8_t *)paging_temp_map(frame);
    for (uint32_t i = 0; i < PAGE_SIZE; i++)
        mapped[i] = 0;
    paging_temp_unmap();

    return frame;
}

static int map_user_page(uint32_t pdt_phys, uint32_t vaddr, uint32_t frame_phys)
{
    uint32_t pd_index = vaddr >> 22;
    uint32_t pt_index = (vaddr >> 12) & 0x3FF;
    uint32_t pt_phys;

    uint32_t *pdt = (uint32_t *)paging_temp_map(pdt_phys);
    uint32_t pde = pdt[pd_index];
    paging_temp_unmap();

    if (!(pde & PDE_PRESENT)) {
        pt_phys = alloc_zeroed_frame();
        if (pt_phys == 0)
            return -1;

        pdt = (uint32_t *)paging_temp_map(pdt_phys);
        pdt[pd_index] = pt_phys | PDE_PRESENT | PDE_WRITABLE | PDE_USER;
        paging_temp_unmap();
    } else {
        pt_phys = pde & 0xFFFFF000;
    }

    uint32_t *pt = (uint32_t *)paging_temp_map(pt_phys);
    pt[pt_index] = (frame_phys & 0xFFFFF000) | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    paging_temp_unmap();

    return 0;
}

static uint32_t create_user_pdt(void)
{
    uint32_t pdt_phys = alloc_zeroed_frame();
    if (pdt_phys == 0)
        return 0;

    uint32_t *pdt = (uint32_t *)paging_temp_map(pdt_phys);
    pdt[KERNEL_PD_INDEX] = page_directory[KERNEL_PD_INDEX];
    paging_temp_unmap();

    return pdt_phys;
}

static int copy_frame_page(uint32_t src_frame_phys, uint32_t dst_frame_phys)
{
    uint8_t bounce[EXEC_COPY_CHUNK];
    uint32_t copied = 0;

    while (copied < PAGE_SIZE) {
        uint32_t chunk = min_u32(PAGE_SIZE - copied, EXEC_COPY_CHUNK);
        uint8_t *src = (uint8_t *)paging_temp_map(src_frame_phys);
        for (uint32_t i = 0; i < chunk; i++)
            bounce[i] = src[copied + i];
        paging_temp_unmap();

        uint8_t *dst = (uint8_t *)paging_temp_map(dst_frame_phys);
        for (uint32_t i = 0; i < chunk; i++)
            dst[copied + i] = bounce[i];
        paging_temp_unmap();

        copied += chunk;
    }

    return 0;
}

static uint32_t clone_user_space(uint32_t parent_pdt_phys)
{
    uint32_t child_pdt_phys = create_user_pdt();

    if (child_pdt_phys == 0)
        return 0;

    for (uint32_t pd_index = 0; pd_index < KERNEL_PD_INDEX; pd_index++) {
        uint32_t *parent_pdt = (uint32_t *)paging_temp_map(parent_pdt_phys);
        uint32_t parent_pde = parent_pdt[pd_index];
        paging_temp_unmap();

        if (!(parent_pde & PDE_PRESENT))
            continue;

        uint32_t parent_pt_phys = parent_pde & 0xFFFFF000;
        uint32_t child_pt_phys = alloc_zeroed_frame();

        if (child_pt_phys == 0) {
            destroy_user_space(child_pdt_phys);
            return 0;
        }

        uint32_t *child_pdt = (uint32_t *)paging_temp_map(child_pdt_phys);
        child_pdt[pd_index] = child_pt_phys | (parent_pde & 0xFFF);
        paging_temp_unmap();

        for (uint32_t pt_index = 0; pt_index < 1024; pt_index++) {
            uint32_t *parent_pt = (uint32_t *)paging_temp_map(parent_pt_phys);
            uint32_t parent_pte = parent_pt[pt_index];
            paging_temp_unmap();

            if (!(parent_pte & PTE_PRESENT))
                continue;

            uint32_t parent_frame = parent_pte & 0xFFFFF000;
            uint32_t child_frame = pfa_alloc();

            if (child_frame == 0) {
                destroy_user_space(child_pdt_phys);
                return 0;
            }

            if (copy_frame_page(parent_frame, child_frame) < 0) {
                pfa_free(child_frame);
                destroy_user_space(child_pdt_phys);
                return 0;
            }

            uint32_t *child_pt = (uint32_t *)paging_temp_map(child_pt_phys);
            child_pt[pt_index] = child_frame | (parent_pte & 0xFFF);
            paging_temp_unmap();
        }
    }

    return child_pdt_phys;
}

static uint32_t clone_current_kernel_stack_page(void)
{
    uint32_t child_kstack_frame = pfa_alloc();

    if (child_kstack_frame == 0)
        return 0;

    uint8_t *dst = (uint8_t *)paging_temp_map(child_kstack_frame);
    uint8_t *src = (uint8_t *)((uint32_t)&kernel_stack_top - PAGE_SIZE);

    for (uint32_t i = 0; i < PAGE_SIZE; i++)
        dst[i] = src[i];

    paging_temp_unmap();
    return child_kstack_frame;
}

static void save_context_from_syscall_frame(struct process_context *ctx,
                                            const struct syscall_frame *frame)
{
    ctx->ebp = frame->ebp;
    ctx->edi = frame->edi;
    ctx->esi = frame->esi;
    ctx->edx = frame->edx;
    ctx->ecx = frame->ecx;
    ctx->ebx = frame->ebx;
    ctx->eax = frame->eax;
    ctx->eip = frame->eip;
    ctx->cs = frame->cs;
    ctx->eflags = frame->eflags;
    ctx->user_esp = frame->user_esp;
    ctx->user_ss = frame->user_ss;
}

static void destroy_user_space(uint32_t pdt_phys)
{
    if (pdt_phys == 0)
        return;

    for (uint32_t pd_index = 0; pd_index < 1024; pd_index++) {
        if (pd_index == KERNEL_PD_INDEX)
            continue;

        uint32_t *pdt = (uint32_t *)paging_temp_map(pdt_phys);
        uint32_t pde = pdt[pd_index];
        paging_temp_unmap();

        if (!(pde & PDE_PRESENT))
            continue;

        uint32_t pt_phys = pde & 0xFFFFF000;
        uint32_t *pt = (uint32_t *)paging_temp_map(pt_phys);

        for (uint32_t pt_index = 0; pt_index < 1024; pt_index++) {
            if (pt[pt_index] & PTE_PRESENT) {
                uint32_t frame_phys = pt[pt_index] & 0xFFFFF000;
                pfa_free(frame_phys);
            }
        }

        paging_temp_unmap();
        pfa_free(pt_phys);

        pdt = (uint32_t *)paging_temp_map(pdt_phys);
        pdt[pd_index] = 0;
        paging_temp_unmap();
    }

    pfa_free(pdt_phys);
}

static int copy_phys_region_to_frame(uint32_t dst_frame_phys, uint32_t src_phys_start, uint32_t size)
{
    uint8_t bounce[EXEC_COPY_CHUNK];
    uint32_t copied = 0;

    if (size > PAGE_SIZE)
        return -1;

    uint8_t *dst = (uint8_t *)paging_temp_map(dst_frame_phys);
    for (uint32_t i = 0; i < PAGE_SIZE; i++)
        dst[i] = 0;
    paging_temp_unmap();

    while (copied < size) {
        uint32_t src_phys = src_phys_start + copied;
        uint32_t src_page = src_phys & 0xFFFFF000;
        uint32_t src_off  = src_phys & 0xFFF;

        uint32_t chunk = min_u32(size - copied, PAGE_SIZE - src_off);
        chunk = min_u32(chunk, EXEC_COPY_CHUNK);

        uint8_t *src = (uint8_t *)paging_temp_map(src_page);
        for (uint32_t i = 0; i < chunk; i++)
            bounce[i] = src[src_off + i];
        paging_temp_unmap();

        dst = (uint8_t *)paging_temp_map(dst_frame_phys);
        for (uint32_t i = 0; i < chunk; i++)
            dst[copied + i] = bounce[i];
        paging_temp_unmap();

        copied += chunk;
    }

    return 0;
}

static int setup_user_stack(uint32_t pdt_phys, uint32_t *user_esp_out)
{
    uint32_t stack_frame = pfa_alloc();
    if (stack_frame == 0)
        return -1;

    if (map_user_page(pdt_phys, USER_STACK_PAGE_VADDR, stack_frame) < 0) {
        pfa_free(stack_frame);
        return -1;
    }

    uint8_t *stack = (uint8_t *)paging_temp_map(stack_frame);
    for (uint32_t i = 0; i < PAGE_SIZE; i++)
        stack[i] = 0;

    /* argc=0 e argv=NULL no topo da stack inicial */
    uint32_t stack_offset = PAGE_SIZE - 8;
    uint32_t *stack_words = (uint32_t *)(stack + stack_offset);
    stack_words[0] = 0;
    stack_words[1] = 0;
    paging_temp_unmap();

    *user_esp_out = USER_STACK_TOP - 8;
    return 0;
}

static int build_new_image(uint32_t mod_start,
                           uint32_t mod_end,
                           uint32_t *new_pdt_out,
                           uint32_t *entry_out,
                           uint32_t *user_esp_out)
{
    uint32_t pdt_phys;
    uint32_t module_size;
    uint32_t loaded;

    if (new_pdt_out == 0 || entry_out == 0 || user_esp_out == 0)
        return -1;

    if (mod_end <= mod_start)
        return -1;

    pdt_phys = create_user_pdt();
    if (pdt_phys == 0)
        return -1;

    module_size = mod_end - mod_start;
    loaded = 0;

    while (loaded < module_size) {
        uint32_t vaddr = USER_TEXT_BASE + loaded;
        uint32_t frame = pfa_alloc();
        uint32_t copy_size = min_u32(PAGE_SIZE, module_size - loaded);

        if (frame == 0) {
            destroy_user_space(pdt_phys);
            return -1;
        }

        if (map_user_page(pdt_phys, vaddr, frame) < 0) {
            pfa_free(frame);
            destroy_user_space(pdt_phys);
            return -1;
        }

        if (copy_phys_region_to_frame(frame, mod_start + loaded, copy_size) < 0) {
            destroy_user_space(pdt_phys);
            return -1;
        }

        loaded += copy_size;
    }

    if (setup_user_stack(pdt_phys, user_esp_out) < 0) {
        destroy_user_space(pdt_phys);
        return -1;
    }

    *new_pdt_out = pdt_phys;
    *entry_out = USER_TEXT_BASE;
    return 0;
}

static uint32_t sys_exec(struct syscall_frame *frame)
{
    char kernel_path[EXEC_PATH_MAX];
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t new_pdt;
    uint32_t new_entry;
    uint32_t new_user_esp;
    struct PCB *pcb;

    const char *user_path = (const char *)frame->ebx;
    if (copy_user_string(user_path, kernel_path, EXEC_PATH_MAX) < 0)
        return (uint32_t)-1;

    if (module_loader_find_module_by_path(kernel_path, &mod_start, &mod_end) < 0) {
        serial_write(SERIAL_COM1_BASE, "[SYSCALL] exec: modulo nao encontrado");
        return (uint32_t)-1;
    }

    pcb = scheduler_get_current();
    if (pcb == 0) {
        serial_write(SERIAL_COM1_BASE, "[SYSCALL] exec: processo corrente ausente");
        return (uint32_t)-1;
    }

    if (build_new_image(mod_start, mod_end, &new_pdt, &new_entry, &new_user_esp) < 0) {
        serial_write(SERIAL_COM1_BASE, "[SYSCALL] exec: falha ao montar nova imagem");
        return (uint32_t)-1;
    }

    uint32_t old_pdt = pcb->pdt;

    pcb->pdt = new_pdt;
    pcb->eip = new_entry;
    pcb->esp = new_user_esp;
    pcb->eflags |= 0x00000200;
    pcb->cs = USER_CODE_SELECTOR_R3;
    pcb->ss = USER_DATA_SELECTOR_R3;

    update_cr3(new_pdt);
    destroy_user_space(old_pdt);

    frame->eip = new_entry;
    frame->user_esp = new_user_esp;
    frame->cs = pcb->cs;
    frame->user_ss = pcb->ss;
    frame->eflags |= 0x00000200;

    /* Hook para escalonador cooperativo: tarefa pronta para próxima seleção. */
    pcb->state = PROCESS_STATE_READY;

    serial_write(SERIAL_COM1_BASE, "[SYSCALL] exec concluido");
    return 0;
}

static uint32_t sys_fork(struct syscall_frame *frame)
{
    struct PCB *parent = scheduler_get_current();
    struct PCB *child;
    uint32_t child_pid;
    uint32_t child_pdt;
    uint32_t child_kstack;

    if (parent == 0) {
        serial_write(SERIAL_COM1_BASE, "[SYSCALL] fork: processo corrente ausente");
        return (uint32_t)-1;
    }

    child = (struct PCB *)kmalloc(sizeof(struct PCB));
    if (child == 0)
        return (uint32_t)-1;

    /* Copia base do PCB do pai e sobreescreve campos dependentes do filho. */
    *child = *parent;

    child_pid = scheduler_allocate_pid();
    child->pid = child_pid;
    child->ppid = parent->pid;
    child->state = PROCESS_STATE_READY;
    child->kernel_stack_frame = 0;
    child->pdt = 0;

    child_pdt = clone_user_space(parent->pdt);
    if (child_pdt == 0) {
        kfree(child);
        serial_write(SERIAL_COM1_BASE, "[SYSCALL] fork: falha ao clonar memoria");
        return (uint32_t)-1;
    }

    child_kstack = clone_current_kernel_stack_page();
    if (child_kstack == 0) {
        destroy_user_space(child_pdt);
        kfree(child);
        serial_write(SERIAL_COM1_BASE, "[SYSCALL] fork: falha ao clonar kstack");
        return (uint32_t)-1;
    }

    child->pdt = child_pdt;
    child->kernel_stack_frame = child_kstack;

    child->eip = frame->eip;
    child->esp = frame->user_esp;
    child->eflags = frame->eflags;
    child->cs = frame->cs;
    child->ss = frame->user_ss;

    save_context_from_syscall_frame(&child->context, frame);
    child->context.eax = 0;

    if (scheduler_enqueue_ready(child) < 0) {
        pfa_free(child_kstack);
        destroy_user_space(child_pdt);
        kfree(child);
        serial_write(SERIAL_COM1_BASE, "[SYSCALL] fork: fila ready cheia");
        return (uint32_t)-1;
    }

    serial_write(SERIAL_COM1_BASE, "[SYSCALL] fork concluido");
    return child_pid;
}

static uint32_t sys_yield(struct syscall_frame *frame)
{
    if (scheduler_schedule_from_context((struct process_context *)frame, 1) < 0) {
        serial_write(SERIAL_COM1_BASE, "[SYSCALL] yield: falha no scheduler");
        return (uint32_t)-1;
    }

    /* Preserve EAX do contexto restaurado no frame para o iret. */
    return frame->eax;
}

uint32_t syscall_dispatcher(uint32_t syscall_number, struct syscall_frame *frame)
{
    serial_write(SERIAL_COM1_BASE, "[SYSCALL] syscall recebida: ");
    serial_write_hex(syscall_number);

    switch (syscall_number) {
        case SYSCALL_TEST:
            serial_write(SERIAL_COM1_BASE, "[SYSCALL] syscall_test chamada!");
            return 0;

        case SYSCALL_WRITE:
            serial_write(SERIAL_COM1_BASE, "[SYSCALL] syscall_write chamada!");
            return 0;

        case SYSCALL_EXEC:
            return sys_exec(frame);

        case SYSCALL_FORK:
            return sys_fork(frame);

        case SYSCALL_YIELD:
            return sys_yield(frame);

        default:
            serial_write(SERIAL_COM1_BASE, "[SYSCALL] syscall desconhecida!");
            return (uint32_t)-1;
    }
}
