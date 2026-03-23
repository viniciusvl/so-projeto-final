# Capítulo 10 — Page Frame Allocation: Paginação e Kernel Heap

## Contexto: O que tínhamos antes

Quando o kernel inicia (via GRUB), o `loader.s` configura a paginação usando **PSE (Page Size Extension)** — páginas de **4MB**:

```
Page Directory (1024 entradas):
  [0]   = 0x00000083  → Identity Map:  virtual 0x00000000 → físico 0x00000000 (4MB)
  [768] = 0x00000083  → Higher Half:   virtual 0xC0000000 → físico 0x00000000 (4MB)
```

Isso significa que o intervalo virtual `0xC0000000 – 0xC03FFFFF` aponta para os mesmos 4MB físicos `0x00000000 – 0x003FFFFF`. É uma mapeamento "grosseiro" — uma entrada cobre 4MB inteiros.

**Problema:** com páginas de 4MB, não temos controle granular. Não podemos:
- Mapear páginas individuais de 4KB
- Usar mapeamento temporário (temp map)
- Expandir o heap página por página

---

## Passo 1: Exportar o Page Directory (`loader.s`)

O Page Directory é definido em assembly (`loader.s`) na seção `.data`:

```asm
section .data
align 4096
page_directory:
    dd 0x00000083               ; entry 0: identity map (4MB, PSE)
    times (768 - 1) dd 0
    dd 0x00000083               ; entry 768: higher half (4MB, PSE)
    times (1024 - 769) dd 0
```

Para que o código C possa modificar esse Page Directory, precisamos exportar o símbolo:

```asm
global page_directory
```

No C, acessamos assim:

```c
extern uint32_t page_directory[];
// page_directory[0]   → entrada 0 (identity map)
// page_directory[768] → entrada 768 (higher half do kernel)
```

O endereço de `page_directory` no C é o **endereço virtual** (ex: `0xC01XXXXX`), porque o linker reloca tudo para `0xC0100000+`. Mas o hardware (CR3) usa o **endereço físico**. O `loader.s` já faz essa conversão: `sub eax, 0xC0000000`.

---

## Passo 2: Criar a Page Table do Kernel (`paging_init`)

### O que é uma Page Table?

```
Hierarquia x86 (sem PSE):

Page Directory (1024 entradas × 4 bytes = 4KB)
  └── Cada entrada aponta para uma Page Table
        Page Table (1024 entradas × 4 bytes = 4KB)
          └── Cada entrada aponta para uma página física de 4KB

Tradução de endereço virtual:
  [ 10 bits PD index | 10 bits PT index | 12 bits offset ]
       bits 31-22         bits 21-12         bits 11-0
```

Para o endereço virtual `0xC00B8000` (framebuffer VGA):
```
PD index = 0xC00B8000 >> 22       = 768
PT index = (0xC00B8000 >> 12) & 0x3FF = 0xB8 = 184
Offset   = 0xC00B8000 & 0xFFF     = 0x000

Então: page_directory[768] → page_table[184] → frame físico 0x000B8000
```

### A Page Table estática

Definimos a page table como um array estático alinhado a 4KB:

```c
static uint32_t kernel_page_table[1024] __attribute__((aligned(4096)));
```

O `__attribute__((aligned(4096)))` garante que o array começa num endereço múltiplo de 4096 (requisito do hardware x86).

### Preenchendo a Page Table

```c
void paging_init(void)
{
    extern uint32_t kernel_physical_end;
    uint32_t kp_end = (uint32_t)&kernel_physical_end;
    uint32_t mapped_frames = (kp_end + PAGE_SIZE - 1) / PAGE_SIZE;
```

`kernel_physical_end` é um símbolo do linker script (`link.ld`). Ele marca o fim do kernel na memória **física**. Usamos `&kernel_physical_end` para obter o valor (em C, símbolos de linker são acessados pelo endereço).

Calculamos quantos frames de 4KB precisamos mapear: de `0x00000000` até `kernel_physical_end`, arredondando para cima.

```c
    for (uint32_t i = 0; i < mapped_frames && i < TEMP_PT_INDEX; i++)
        kernel_page_table[i] = (i * PAGE_SIZE) | PTE_PRESENT | PTE_WRITABLE;
```

Cada entrada `i` mapeia:
- **Virtual:** `0xC0000000 + i * 4096`
- **Físico:** `i * 4096`

Os flags:
- `PTE_PRESENT (0x01)` — página existe na memória
- `PTE_WRITABLE (0x02)` — pode escrever

Isso cobre:
| Range Físico | Range Virtual | Conteúdo |
|---|---|---|
| `0x00000 – 0x9FFFF` | `0xC0000000 – 0xC009FFFF` | Memória convencional (640KB) |
| `0xA0000 – 0xBFFFF` | `0xC00A0000 – 0xC00BFFFF` | VGA (framebuffer em 0xB8000) |
| `0xC0000 – 0xFFFFF` | `0xC00C0000 – 0xC00FFFFF` | BIOS ROM |
| `0x100000 – kernel_end` | `0xC0100000 – 0xC01XXXXX` | Código e dados do kernel |

```c
    for (uint32_t i = mapped_frames; i < TEMP_PT_INDEX; i++)
        kernel_page_table[i] = 0;
```

As entradas acima do kernel ficam **não-presentes** (valor 0). O heap vai mapeá-las sob demanda.

```c
    kernel_page_table[TEMP_PT_INDEX] = 0;   // entrada 1023: reservada para temp map
```

A entrada 1023 é especial — reservada para mapeamento temporário (explicado no Passo 3).

### Instalando a Page Table no Page Directory

```c
    uint32_t pt_phys = (uint32_t)kernel_page_table - 0xC0000000;
    page_directory[KERNEL_PD_INDEX] = pt_phys | PDE_PRESENT | PDE_WRITABLE;
    reload_cr3();
```

1. **Calcula endereço físico:** como o kernel está em `virtual = physical + 0xC0000000`, subtraímos o offset
2. **Escreve na entrada 768:** o endereço físico da page table + flags (presente, escrita)
3. **Flush TLB:** recarregamos CR3 para invalidar todos os caches de tradução

```c
static inline void reload_cr3(void)
{
    uint32_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}
```

Lê CR3 e escreve de volta — isso força o processador a descartar o TLB inteiro.

### Antes vs Depois

```
ANTES (PSE):
  page_directory[768] = 0x00000083
  Bit 7 (PS) = 1 → página de 4MB
  Mapeia 0xC0000000–0xC03FFFFF → 0x00000000–0x003FFFFF (bloco único)

DEPOIS (Page Table):
  page_directory[768] = endereço_físico_da_page_table | 0x03
  Bit 7 (PS) = 0 → aponta para page table
  Cada entrada da page table mapeia 4KB individualmente
```

---

## Passo 3: Mapeamento Temporário (Temp Map)

### O problema circular

O PDF descreve este problema:

> O page frame allocator retorna um endereço **físico**. Mas para escrever nesse frame, ele precisa estar **mapeado** no espaço virtual. E se precisarmos de uma nova page table? Ela ocupa um frame inteiro, e para inicializá-la (zerar 4KB), precisamos mapeá-la primeiro... mas ela É a page table que usaríamos para mapear!

### A solução: entrada 1023

Reservamos a **última entrada** (1023) da page table do kernel para mapeamentos temporários:

```
Endereço virtual do temp map:
  PD index = 768, PT index = 1023, offset = 0
  (768 << 22) | (1023 << 12) | 0x000 = 0xC03FF000
```

Então o endereço virtual fixo `0xC03FF000` pode apontar para **qualquer** frame físico temporariamente.

### Como funciona

```c
void *paging_temp_map(uint32_t paddr)
{
    kernel_page_table[TEMP_PT_INDEX] = (paddr & 0xFFFFF000) | PTE_PRESENT | PTE_WRITABLE;
    invlpg(TEMP_MAP_VADDR);
    return (void *)TEMP_MAP_VADDR;
}
```

1. Escreve o endereço físico desejado na entrada 1023 da page table
2. `invlpg` invalida apenas a entrada TLB para `0xC03FF000` (mais eficiente que recarregar CR3 inteiro)
3. Retorna ponteiro para `0xC03FF000` — agora aponta para o frame físico

```c
static inline void invlpg(uint32_t addr)
{
    asm volatile("invlpg (%0)" : : "r"(addr) : "memory");
}
```

A instrução `invlpg` invalida uma única página no TLB. A sintaxe AT&T usa `(%0)` para dereferenciar o registrador.

### Uso prático: criando uma nova page table

Quando `paging_map_page` precisa mapear um endereço fora do range do kernel (ex: entry 769 do PD):

```c
// 1. Aloca um frame físico para a nova page table
uint32_t pt_frame = pfa_alloc();

// 2. Mapeia temporariamente para poder escrever nela
uint32_t *new_pt = (uint32_t *)paging_temp_map(pt_frame);

// 3. Zera todas as 1024 entradas
for (uint32_t i = 0; i < 1024; i++)
    new_pt[i] = 0;

// 4. Configura a entrada desejada
new_pt[pt_index] = paddr | flags;

// 5. Remove o mapeamento temporário
paging_temp_unmap();

// 6. Instala a page table no Page Directory
page_directory[pd_index] = pt_frame | PDE_PRESENT | PDE_WRITABLE;
```

Isso quebra a dependência circular: usamos a entrada 1023 (que já está na page table do kernel) para acessar temporariamente o frame da nova page table.

---

## Passo 4: Mapeamento de Páginas (`paging_map_page`)

```c
void paging_map_page(uint32_t vaddr, uint32_t paddr, uint32_t flags)
{
    uint32_t pd_index = vaddr >> 22;           // bits 31-22
    uint32_t pt_index = (vaddr >> 12) & 0x3FF; // bits 21-12
```

Decompomos o endereço virtual nos seus componentes:

```
Exemplo: vaddr = 0xC0130000
  pd_index = 0xC0130000 >> 22 = 768
  pt_index = (0xC0130000 >> 12) & 0x3FF = 0x130 = 304
```

### Caso 1: dentro da page table do kernel (pd_index == 768)

```c
    if (pd_index == KERNEL_PD_INDEX) {
        kernel_page_table[pt_index] = (paddr & 0xFFFFF000) | flags;
        invlpg(vaddr);
        return;
    }
```

Acesso direto — a `kernel_page_table` é uma variável estática acessível. Apenas escrevemos o endereço físico (alinhado a 4KB) com os flags e invalidamos a entrada TLB.

### Caso 2: fora do range do kernel

Usa o mapeamento temporário para acessar/criar a page table correspondente (explicado no Passo 3).

---

## Passo 5: Kernel Heap — `ksbrk` (substituto de sbrk)

### O que é sbrk?

Em sistemas Unix, `sbrk(n)` expande o heap do processo por `n` bytes, retornando o ponteiro antigo. No nosso kernel, não temos sistema operacional por baixo — nós **somos** o sistema operacional. Então substituímos `sbrk` por `ksbrk`:

```c
static uint32_t heap_current;   // próximo endereço virtual livre
static uint32_t heap_max;       // limite: 0xC03FF000

static void *ksbrk(uint32_t nbytes)
{
    nbytes = (nbytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
```

Arredonda para múltiplo de `PAGE_SIZE` (4096). Exemplo: pedir 100 bytes → aloca 4096 bytes (1 página).

```c
    if (heap_current + nbytes > heap_max)
        return (void *)(uint32_t)-1;   // sem espaço
```

O heap está limitado a `0xC03FF000` (a última página é reservada para temp map).

```c
    uint32_t old = heap_current;

    for (uint32_t addr = heap_current; addr < heap_current + nbytes; addr += PAGE_SIZE) {
        uint32_t frame = pfa_alloc();           // 1. pega frame físico
        if (frame == 0)
            return (void *)(uint32_t)-1;
        paging_map_page(addr, frame,            // 2. mapeia virtual → físico
                        PTE_PRESENT | PTE_WRITABLE);
    }

    heap_current += nbytes;
    return (void *)old;
}
```

Para cada página necessária:
1. `pfa_alloc()` retorna um endereço **físico** de um frame livre
2. `paging_map_page()` cria o mapeamento na page table do kernel

Note que o endereço **virtual** é sequencial (heap_current, heap_current+4096, ...) mas os frames **físicos** podem estar em qualquer lugar da memória. Isso é a essência da memória virtual.

### Inicialização do Heap

```c
void kheap_init(void)
{
    extern uint32_t kernel_virtual_end;
    heap_current = ((uint32_t)&kernel_virtual_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    heap_max     = TEMP_MAP_VADDR;
}
```

O heap começa logo após o fim do kernel (alinhado a 4KB) e pode crescer até `0xC03FF000`.

```
Layout do espaço virtual do kernel (entry 768 do PD):

0xC0000000  ┌──────────────────┐
            │  BIOS / VGA etc  │  ← mapeado em paging_init
0xC0100000  ├──────────────────┤
            │  Código do kernel│  ← .text, .rodata, .data, .bss
            │  (inclui PFA     │
            │   bitmap, page   │
            │   table, stack)  │
kernel_end  ├──────────────────┤
            │                  │
            │    HEAP          │  ← cresce para cima via ksbrk
            │   (kmalloc)      │
            │                  │
0xC03FF000  ├──────────────────┤
            │  TEMP MAP (4KB)  │  ← entrada 1023 da page table
0xC0400000  └──────────────────┘
```

---

## Passo 6: kmalloc e kfree — Algoritmo K&R

Baseado no livro "The C Programming Language" de Kernighan & Ritchie (capítulo 8.7).

### Estrutura de dados

```c
typedef struct header {
    struct header *next;   // próximo bloco livre
    uint32_t       size;   // tamanho em unidades de sizeof(Header) = 8 bytes
} Header;
```

Cada bloco de memória (livre ou alocado) tem um cabeçalho de 8 bytes:
- `next` — ponteiro para o próximo bloco livre (lista circular)
- `size` — tamanho total do bloco (cabeçalho + dados) em unidades de 8 bytes

```
Bloco alocado (ex: kmalloc(20)):
  ┌─────────┬──────────────────────────┐
  │ Header  │      Dados (20 bytes)    │  padding...
  │ 8 bytes │                          │
  └─────────┴──────────────────────────┘
  ↑           ↑
  bp          ptr (retornado ao usuário)

  size = (20 + 7) / 8 + 1 = 4 unidades = 32 bytes total
```

### Lista circular de blocos livres

```
freep → [base] → [bloco A] → [bloco B] → [base] → ...
          size=0    size=50     size=200

A lista é ordenada por endereço de memória.
"base" é uma sentinela com size=0 que nunca é alocada.
```

### kmalloc — alocação

```c
void *kmalloc(uint32_t nbytes)
{
    uint32_t nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;
```

Converte bytes para "unidades" (cada unidade = 8 bytes). O `+1` conta o próprio Header.

```c
    Header *prevp = freep;
    if (prevp == 0) {
        base.next = &base;
        base.size = 0;
        freep = prevp = &base;
    }
```

Na primeira chamada, inicializa a lista circular com a sentinela `base`.

```c
    for (Header *p = prevp->next; ; prevp = p, p = p->next) {
        if (p->size >= nunits) {
```

Percorre a lista circular procurando um bloco grande o suficiente.

```c
            if (p->size == nunits) {
                prevp->next = p->next;    // tamanho exato: remove da lista
            } else {
                p->size -= nunits;        // maior: corta do final
                p += p->size;
                p->size = nunits;
            }
            freep = prevp;
            return (void *)(p + 1);       // retorna ponteiro APÓS o header
        }
```

Se o bloco é exato, remove da lista. Se é maior, aloca do **final** do bloco (evita mover ponteiros).

```c
        if (p == freep) {
            p = morecore(nunits);         // voltou ao início: pede mais memória
            if (p == 0)
                return 0;                 // sem memória
        }
    }
}
```

Se percorreu a lista inteira sem achar, chama `morecore` para expandir o heap.

### morecore — pedir mais memória

```c
static Header *morecore(uint32_t nunits)
{
    if (nunits < NALLOC)
        nunits = NALLOC;                  // mínimo 128 unidades = 1024 bytes
```

Evita chamadas frequentes a `ksbrk` — pede pelo menos `NALLOC` unidades de uma vez.

```c
    uint32_t nbytes = nunits * sizeof(Header);
    nbytes = (nbytes + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);  // arredonda para página
```

`ksbrk` trabalha com páginas inteiras, então arredondamos.

```c
    void *p = ksbrk(nbytes);
    if (p == (void *)(uint32_t)-1)
        return 0;

    Header *hp = (Header *)p;
    hp->size = nbytes / sizeof(Header);   // tamanho real (pode ser mais que pedido)
    kfree((void *)(hp + 1));              // insere na lista livre
    return freep;
}
```

1. Chama `ksbrk` para mapear novas páginas
2. Cria um Header no início do bloco com o tamanho real
3. Chama `kfree` para inserir o bloco na lista livre (reutiliza a lógica de inserção)
4. Retorna `freep` — o `kmalloc` vai tentar alocar de novo e agora vai encontrar o bloco

### kfree — liberação com coalescing

```c
void kfree(void *ptr)
{
    Header *bp = (Header *)ptr - 1;   // volta ao header (1 unidade antes)
    Header *p;
```

O ponteiro retornado por `kmalloc` é `header + 1`, então subtraímos 1 para achar o header.

```c
    for (p = freep; !(bp > p && bp < p->next); p = p->next) {
        if (p >= p->next && (bp > p || bp < p->next))
            break;
    }
```

Encontra a posição correta na lista (ordenada por endereço). O `break` trata o caso das bordas (inserir no início ou fim da lista circular).

```c
    // Coalesce com vizinho superior
    if (bp + bp->size == p->next) {
        bp->size += p->next->size;    // junta os tamanhos
        bp->next  = p->next->next;    // pula o vizinho
    } else {
        bp->next = p->next;
    }
```

Se o bloco liberado é **adjacente** ao próximo bloco livre, funde os dois em um só. Isso evita fragmentação.

```c
    // Coalesce com vizinho inferior
    if (p + p->size == bp) {
        p->size += bp->size;          // junta os tamanhos
        p->next  = bp->next;          // absorve bp
    } else {
        p->next = bp;
    }

    freep = p;
}
```

Mesma lógica para o bloco anterior. No melhor caso, três blocos adjacentes viram um só.

```
Antes do kfree(B):
  [A livre] [B usado] [C livre]

Depois do kfree(B) com coalescing:
  [────── A+B+C livre ──────]
```

---

## Resumo: Fluxo completo de um `kmalloc(100)`

```
1. kmalloc(100)
   └── nunits = (100 + 7) / 8 + 1 = 14 unidades (112 bytes)

2. Percorre lista livre → não encontra bloco

3. morecore(128)              ← mínimo NALLOC=128 unidades
   └── nbytes = 128 × 8 = 1024 → arredonda para 4096 (1 página)

4. ksbrk(4096)
   ├── pfa_alloc() → frame físico 0x00135000
   ├── paging_map_page(0xC0130000, 0x00135000, PRESENT|WRITABLE)
   │   └── kernel_page_table[0x130] = 0x00135003
   └── heap_current = 0xC0131000

5. kfree() insere bloco de 512 unidades (4096/8) na lista livre

6. kmalloc encontra o bloco, aloca 14 unidades do final
   ├── Bloco livre: 512 - 14 = 498 unidades restam
   └── Retorna ponteiro 0xC0130F90 (exemplo)

7. Usuário escreve ptr[0] = 0xCAFEBABE
   └── CPU traduz 0xC0130F90 → page_directory[768] → kernel_page_table[0x130]
       → frame 0x00135000 + offset 0xF90 → físico 0x00135F90
```
