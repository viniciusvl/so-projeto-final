## **Descrição:**
  Desenvolvimento do Kernel de um sistema operacional, como projeto final à disciplina Sistemas Operacionais 
  
  - **Período**: 2025.2
  - **Professor**: Davi Henrique dos Santos

## Organização dos arquivos:

```text
so-projeto-final/
├── build/                  # Diretório de compilação (arquivos .o)
├── out/                    # Diretório de saída (ISO gerada)
├── iso/
│   └── boot/
│       ├── grub/
│       │   └── stage2_eltorito     # Binário do GRUB
│       └── kernel.elf             # Kernel compilado
├── include/                # Headers do projeto
│   ├── types.h             # Definições de tipos
│   ├── multiboot.h         # Estruturas do Multiboot
│   ├── io/
│   │   └── io.h            # Declaração das funções de IO
│   ├── kernel/
│   │   └── module_loader.h # Carregamento de módulos GRUB
│   ├── segment/
│   │   └── gdt.h           # Definição das structs da GDT
│   └── interrupts/
│       └── idt.h           # Definição das structs da IDT
├── linker/
│   └── link.ld             # Script do linker
├── config/
│   └── bochsrc.txt         # Configuração do emulador Bochs
├── src/
│   ├── boot/
│   │   └── loader.s        # Ponto de entrada do kernel (Assembly)
│   ├── kernel/
│   │   ├── kmain.c         # Função principal do kernel (C)
│   │   └── module_loader.c # Carregamento de módulos GRUB (C)
│   ├── io/
│   │   ├── outb.s          # Implementação de outb (Assembly)
│   │   ├── inb.s           # Implementação de inb (Assembly)
│   │   ├── io.c            # Funções do framebuffer (C)
│   │   └── serial_ports.c  # Comunicação via portas seriais (C)
│   ├── segment/
│   │   ├── gdt.c           # Inicialização da GDT (C)
│   │   ├── lgdt_f.s        # Carregamento da GDT (Assembly)
│   │   ├── segment_selector.s  # Config dos registradores de segmento (Assembly)
│   │   └── far_jump.s      # Far jump para atualizar CS (Assembly)
│   └── interrupts/
│       ├── idt.c           # Inicialização dos descritores da IDT (C)
│       ├── interrupt_handler.s # Handler comum de interrupções (Assembly)
│       ├── interrupt_handler.c # Dispatch das interrupções (C)
│       ├── load_lidt.s     # Carregamento da IDT (Assembly)
│       ├── pic.c           # Inicialização do PIC (C)
│       └── keyboard.c      # Driver de teclado (C)
├── Makefile                # Script de compilação
├── README.md               # Este arquivo
└── .gitignore
```

### Descrição dos módulos:

#### **Boot e Kernel**
- **`loader.s`** (`src/boot/`): Código Assembly que configura paginação (Identity Mapping + PSE de 4MB), inicializa a pilha no higher half (3GB) e chama `kmain`.
- **`kmain.c`** (`src/kernel/`): Função principal do kernel, responsável por inicializar GDT, IDT, PIC e executar módulos GRUB.
- **`module_loader.c`** (`src/kernel/`): Carregador de módulos fornecidos pelo bootloader GRUB.

#### **Headers e Tipos**
- **`types.h`**: Definições de tipos primitivos.
- **`multiboot.h`**: Estruturas de dados do padrão Multiboot.

#### **I/O (Entrada/Saída)**
- **`io.h` / `io.c`**: Funções para escrita no framebuffer (caracteres e cores), movimento de cursor.
- **`outb.s` / `inb.s`**: Implementação Assembly para envio/leitura de dados em portas I/O.
- **`serial_ports.c`**: Configuração e comunicação via portas seriais (COM1).

#### **Segmentação de Memória (GDT)**
- **`gdt.h` / `gdt.c`**: Inicialização dos descritores de segmento da Global Descriptor Table.
- **`lgdt_f.s`**: Instrução Assembly `lgdt` para carregar a GDT.
- **`segment_selector.s`**: Configuração dos registradores de segmento (DS, SS, ES).
- **`far_jump.s`**: Far jump para atualizar o registrador CS (Code Segment).

#### **Interrupções (IDT)**
- **`idt.h` / `idt.c`**: Inicialização dos descritores da Interrupt Descriptor Table.
- **`load_lidt.s`**: Instrução Assembly `lidt` para carregar a IDT.
- **`interrupt_handler.s`**: Handler genérico de interrupções (Assembly).
- **`interrupt_handler.c`**: Dispatch das interrupções para handlers específicos.
- **`pic.c`**: Inicialização e acknowledge do Programmable Interrupt Controller.
- **`keyboard.c`**: Driver de teclado para converter scan codes em ASCII.


## Requisitos e Dependências:

Para compilar e executar este projeto, você precisa de:

### Para compilação:
- **gcc** - Compilador C (arquitetura i386/32-bit)
- **nasm** - Assembler para código Assembly
- **ld** - GNU Linker

### Para criar a ISO:
- **genisoimage** - Ferramenta para criar imagens ISO

### Para executar/emular:
- **bochs** - Emulador x86

### No Linux (Debian/Ubuntu):
```bash
sudo apt-get install gcc-multilib nasm binutils genisoimage bochs bochs-x
```

### No Windows:
Recomenda-se usar WSL (Windows Subsystem for Linux) e instalar as dependências acima.

---

## Fluxo de Execução:

```
1. loader.s (Assembly)
   ├─> Configura Page Directory (Identity Mapping + higher half)
   ├─> Habilita PSE (páginas de 4MB no CR4)
   ├─> Ativa paginação (CR0 bit 31)
   ├─> Salta para higher half (0xC0000000+)
   ├─> Inicializa a pilha no kernel space
   └─> Chama kmain()

2. kmain.c (C)
   ├─> get_module_list() - Carrega módulos do GRUB
   ├─> init_gdt() - Inicializa segmentação de memória
   ├─> init_idt_desc() - Inicializa interrupções (IDT)
   ├─> pic_init() - Inicializa o controlador de interrupções
   ├─> outb() - Configura máscara de interrupções do PIC
   ├─> fb_write() - Escreve mensagem na tela
   ├─> sti - Habilita interrupções
   └─> run_module() - Executa módulo GRUB carregado

3. Quando uma interrupção ocorre:
   └─> interrupt_handler_33 (Assembly) → interrupt_handler() (C) → keyboard_handler()
       └─> Converte scan code em ASCII
       └─> Escreve caractere no framebuffer via fb_write_cell()
```

### Módulos Compilados:
- `build/loader.o` - Bootloader Assembly
- `build/kmain.o` - Kernel principal
- `build/io_c.o` - Funções de IO (framebuffer e serial)
- `build/gdt.o` - Segmentação
- `build/idt.o`, `build/interrupt_handler.o`, `build/keyboard.o` - Interrupções
- `build/pic.o` - Controlador de interrupções
- `build/module_loader.o` - Carregador de módulos
- Assembly helpers: `lgdt_f.o`, `load_lidt.o`, `far_jump.o`, etc.

## **Instruções de compilação e execução:**

**Para compilar o projeto:**
```text
make
```

**Para rodar o projeto:**
```text
make run
```

**Para limpar os arquivos compilados:**
```text
make clean
```

## **Constribuições individuais:**
