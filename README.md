## **Descrição:**
  Desenvolvimento do Kernel de um sistema operacional, como projeto final à disciplina Sistemas Operacionais 
  
  - **Período**: 2025.2
  - **Professor**: Davi Henrique dos Santos

## Organização dos arquivos:

```text
└── src/
    ├── loader.s            # Ponto de entrada do kernel
    ├── kmain.c             # Função chamada pelo loader
    ├── link.ld             # Script do linker 
    ├── bochsrc.txt         # Configuração do emulador Bochs
    ├── stage2_eltorito     # Binário do GRUB 
    ├── io/
    │   ├── outb.s          # Implementação da função outb (Assembly)
    │   ├── inb.s           # Implementação da função inb (Assembly)
    │   ├── io.c            # Funções do framebuffer 
    │   ├── io.h            # Declaração das funções de IO
    │   └── serial_ports.c  # Funções de comunicação via portas seriais
    ├── segment/
        ├── gdt.h               # Definição das structs da GDT
        ├── gdt.c               # Inicialização dos descritores e da GDT
        ├── lgdt_f.s            # Função Assembly que carrega a GDT (lgdt)
        ├── segment_selector.s  # Configuração dos registradores de segmento
        └── far_jump.s          # Far jump para atualizar o registrador CS
    └── interrupts/
        ├── idt.h               # Definição das structs da IDT e declarações
        ├── idt.c               # Função de inicialização dos descritores da IDT
        ├── interrupt_handler.s # Macro e handler comum de interrupções (Assembly)
        ├── interrupt_handler.c # Dispatch das interrupções para os handlers em C
        ├── load_lidt.s         # Função Assembly que carrega a IDT (lidt)
        ├── pic.c               # Inicialização e acknowledge do PIC
        └── keyboard.c          # Driver de teclado (scan code → ASCII)
```

- **`loader.s`**: Código Assembly que inicializa a pilha do kernel e chama a função `kmain`.
- **`kmain.c`**: Contém a função principal do kernel, que é chamada após o boot.
- **`link.ld`**: Define como o linker organiza as seções do kernel na memória.
- **`io/`**: Módulo de entrada e saída, responsável pela comunicação com o framebuffer (escrita de caracteres na tela e movimentação do cursor) e com portas seriais.
- **`segment/`**: Módulo de segmentação de memória, responsável pela configuração da Global Descriptor Table (GDT), carregamento via `lgdt` e atualização dos segment selectors.
- **`interrupts/`**: Módulo de interrupções, responsável pela configuração da IDT, inicialização do PIC, e tratamento de interrupções de hardware (teclado).


## **Instruções de compilação e execução:**

**Para compilar o projeto:**
```text
make
```
**Para rodar o projeto:**
```text
make run 
```

## **Constribuições individuais:**
