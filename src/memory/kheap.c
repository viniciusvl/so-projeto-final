/*
 * Kernel Heap — implementação baseada em K&R (Kernighan & Ritchie)
 *
 * Substitui sbrk/brk por chamadas ao Page Frame Allocator (pfa_alloc)
 * e mapeia os frames alocados no espaço virtual do kernel via paging_map_page.
 */

#include "memory/kheap.h"
#include "memory/pfa.h"
#include "memory/paging.h"

/* ---- Cabeçalho de bloco (K&R) ---- */

typedef struct header {
    struct header *next;   /* próximo bloco livre na lista circular */
    uint32_t       size;   /* tamanho em unidades de sizeof(Header)  */
} Header;

/* ---- Estado do heap ---- */

static Header   base;           /* sentinela da lista circular */
static Header  *freep = 0;      /* início da lista de blocos livres */

static uint32_t heap_current;   /* próximo endereço virtual disponível */
static uint32_t heap_max;       /* limite: 0xC03FF000 (temp map)      */

#define NALLOC 128              /* unidades mínimas por expansão */

/* ---- ksbrk: expande o heap mapeando novos frames ---- */

static void *ksbrk(uint32_t nbytes)
{
    /* Arredonda para múltiplo de PAGE_SIZE */
    nbytes = (nbytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    if (heap_current + nbytes > heap_max)
        return (void *)(uint32_t)-1;

    uint32_t old = heap_current;

    for (uint32_t addr = heap_current; addr < heap_current + nbytes; addr += PAGE_SIZE) {
        uint32_t frame = pfa_alloc();
        if (frame == 0)
            return (void *)(uint32_t)-1;
        paging_map_page(addr, frame, PTE_PRESENT | PTE_WRITABLE);
    }

    heap_current += nbytes;
    return (void *)old;
}

/* ---- morecore: pede mais memória ao sistema (K&R) ---- */

static Header *morecore(uint32_t nunits)
{
    if (nunits < NALLOC)
        nunits = NALLOC;

    uint32_t nbytes = nunits * sizeof(Header);
    /* Arredonda para página inteira; recalcula unidades reais */
    nbytes = (nbytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    void *p = ksbrk(nbytes);
    if (p == (void *)(uint32_t)-1)
        return 0;

    Header *hp = (Header *)p;
    hp->size = nbytes / sizeof(Header);
    kfree((void *)(hp + 1));   /* insere na lista livre */
    return freep;
}

void kheap_init(void)
{
    extern uint32_t kernel_virtual_end;
    heap_current = ((uint32_t)&kernel_virtual_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    heap_max     = TEMP_MAP_VADDR;   /* 0xC03FF000 */
}

void *kmalloc(uint32_t nbytes)
{
    if (nbytes == 0)
        return 0;

    uint32_t nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

    Header *prevp = freep;
    if (prevp == 0) {
        /* Primeira chamada: inicializa lista circular vazia */
        base.next = &base;
        base.size = 0;
        freep = prevp = &base;
    }

    for (Header *p = prevp->next; ; prevp = p, p = p->next) {
        if (p->size >= nunits) {
            if (p->size == nunits) {
                /* Bloco exato: remove da lista */
                prevp->next = p->next;
            } else {
                /* Bloco maior: aloca no final */
                p->size -= nunits;
                p += p->size;
                p->size = nunits;
            }
            freep = prevp;
            return (void *)(p + 1);
        }
        if (p == freep) {
            /* Deu a volta inteira sem achar: pede mais memória */
            p = morecore(nunits);
            if (p == 0)
                return 0;
        }
    }
}

void kfree(void *ptr)
{
    if (ptr == 0)
        return;

    Header *bp = (Header *)ptr - 1;   /* cabeçalho do bloco */
    Header *p;

    /* Encontra a posição na lista livre (ordenada por endereço) */
    for (p = freep; !(bp > p && bp < p->next); p = p->next) {
        if (p >= p->next && (bp > p || bp < p->next))
            break;   /* bp antes do início ou depois do fim da lista */
    }

    /* Coalesce com vizinho superior */
    if (bp + bp->size == p->next) {
        bp->size += p->next->size;
        bp->next  = p->next->next;
    } else {
        bp->next = p->next;
    }

    /* Coalesce com vizinho inferior */
    if (p + p->size == bp) {
        p->size += bp->size;
        p->next  = bp->next;
    } else {
        p->next = bp;
    }

    freep = p;
}
