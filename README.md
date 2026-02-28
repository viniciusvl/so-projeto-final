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
    └── io/
        ├── io.s            # Implementação da função outb (Assembly)
        ├── io.c            # Funções do framebuffer 
        └── io.h            # Declaração das funções
```

- **`loader.s`**: Código Assembly que inicializa a pilha do kernel e chama a função `kmain`.
- **`kmain.c`**: Contém a função principal do kernel, que é chamada após o boot.
- **`link.ld`**: Define como o linker organiza as seções do kernel na memória.
- **`io/`**: Módulo de entrada e saída, responsável pela comunicação com o framebuffer (escrita de caracteres na tela e movimentação do cursor).


## **Instruções de compilação e execução:**

**Para compilar o projeto:**
```text
make
```
**Para rodar o projeto:**
```text
make run 
```
**Para limpar arquivos .elf, .o e iso, desnecessários para rodar o projeto**
```text
make clean
```

## **Constribuições individuais:**
