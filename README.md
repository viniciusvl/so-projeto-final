## **Descrição:**
  Desenvolvimento do Kernel de um sistema operacional, como projeto final à disciplina Sistemas Operacionais 
  
  - **Período**: 2025.2
  - **Professor**: Davi Henrique dos Santos


## **Requisitos e Dependências**:

Para instalar as dependências do projeto:

### No Linux (Debian/Ubuntu):
```bash
sudo apt-get install gcc-multilib nasm binutils genisoimage bochs bochs-x
```

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


## **Constribuições individuais:**

**VINICIUS VICTOR LOPES DE ALMEIDA, 20240008181**

- **Capítulo 5:** segmentação de memória com GDT apenas kernel. 
- **Capítulo 6:** leitura de interrupções do teclado
- **Capítulo 11:** sistema para que processos rodem em User Mode
- **Capítulo 13:** permitir que processos em User Mode lancem interrupções para fazer chamadas de sistema 

**PEDRO HENRIQUE PAIVA SOUZA, 20240008145**:

- **Capítulo 2:** configurações iniciais do projeto
- **Capítulo 3:** configurações de pilha do Kernel e integração do código C
- **Capítulo 7:** rodar programas que estão em módulos do GRUB, pré User Mode
- **Capítulo 14:** sistema multitasking 
- **Algoritmo de escalonamento FCFS**

**MARIA ELOIZA MONTEIRO DE SOUZA, 20240008280**:

- **Capítulo 9:** ativar paginação no sistema
- **Capítulo 10:** criar Page Frame Allocator, criar Kernel Heap, implemetar função de *malloc*  

**ALLAN GABRIEL DA CUNHA VASCONCELOS, 20240009142**:

- **Capítulo 4:** sistema de I/O, framebuffer e serial port. 
- **Algoritmo de escalonamento SJF**
- **Algoritmo de escalonamento Round Robin**