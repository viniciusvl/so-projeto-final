/* Funções de C */

#include "io/io.h"

void kmain() {
  serial_write(SERIAL_COM1_BASE, 'A');
}