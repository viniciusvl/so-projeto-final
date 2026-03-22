/*  Esse arquivo implementa funções que conecta um dispositivo
com o outro através de Serial Ports.
*/

#include "io/io.h"
#include "io/serial_ports.h"

/* Essa sintaxe permite chamar um macro passando
um valor como parâmetro e ele retorna o valor modificado
com uma função declarada no segundo parênteses */
#define SERIAL_DATA_PORT(base) (base)
#define SERIAL_FIFO_COMMAND_PORT(base) (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base) (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base) (base + 5)

// Valor para indicar que os próximos 8 bits são os mais altos
#define SERIAL_LINE_ENABLE_DLAB 0x80

// Configura velocidade de transmissão
void serial_configure_baud_rate(unsigned short com, unsigned short divisor) {
  /* Avisa que os próximos 8 bits são da parte alta */
  outb(SERIAL_LINE_COMMAND_PORT(com), SERIAL_LINE_ENABLE_DLAB);
  /* Divisor é um número usado para configurar a velocidade da transmissão
     Divisor é um número de 16 bits. Por isso que é necessário inverter */
  outb(SERIAL_DATA_PORT(com), (divisor >> 8) & 0x00FF);
  /* Envia o resto dos bits do divisor */
  outb(SERIAL_DATA_PORT(com), divisor & 0x00FF);
}

void serial_configure_line(unsigned short com) {
  // Configura opções de verificações e comportamentos específicos
  outb(SERIAL_LINE_COMMAND_PORT(com), 0x03);
}

void serial_configure_buffer(unsigned short com) {
  /* Todo dispositivo possui um buffer para armazenar dados
  enviados ou recebidos
  Essa fila possui várias configurações:

  Bit:     | 7 6 | 5 | 4 | 3 | 2 | 1 | 0 |
  Content: | lvl | bs| r | dma | clt | clr | e |

  lvl: quantidade de bytes que podem ser armazenados
  bs: largura dos buffers
  r: reservado
  dma: forma que o dado é acessado
  clt: limpar a fila de transmissão do buffer
  clr: limpar o buffer recebedor da fila
  e: se esta ativa ou nao
  */

  // Fila ativa, limpa fila e usa 14 bytes de tamanho
  outb(SERIAL_FIFO_COMMAND_PORT(com), 0xC7);
}

void serial_configure_modem(unsigned short com) {
  /*Controla quando o dispositivo está pronto para receber
  ou enviar mensagens

  Com essa configuração atual, setamos o dispositivo para
  "pronto para enviar"
  */

  outb(SERIAL_MODEM_COMMAND_PORT(com), 0x03);
}

int serial_is_transmit_fifo_empty(unsigned int com) {
  return inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;
}

// recebo um endereço de porta e uso a função out para escrever
void serial_write(unsigned short com, char *log) {
  while (serial_is_transmit_fifo_empty(com) == 0) {
    ;
  }
  serial_configure_baud_rate(com, 2);
  serial_configure_line(com);
  serial_configure_buffer(com);
  serial_configure_modem(com);

  for (char *l = log; *l != '\0'; l++) {
    while (serial_is_transmit_fifo_empty(com) == 0) {
      ;
    }
    outb(SERIAL_DATA_PORT(com), *l);
  }

  outb(SERIAL_DATA_PORT(com), '\n');
}
void serial_write_hex(uint32_t num)
{
    char hex[] = "0123456789ABCDEF";
    char buffer[9];

    for (int i = 0; i < 8; i++) {
        buffer[7 - i] = hex[num & 0xF];
        num >>= 4;
    }

    buffer[8] = '\0';

    serial_write(SERIAL_COM1_BASE, buffer);
}