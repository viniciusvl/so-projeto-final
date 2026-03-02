#include "../io/io.h"
#include "idt.h"

#define KBD_DATA_PORT 0x60

/* Tabela de scan codes para ASCII (teclado US) */
static unsigned char scancode_to_ascii[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,
    9,   'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 10,  0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39,  '`', 0,   92,  'z',
    'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' '};

unsigned char read_scan_code(void) { return inb(KBD_DATA_PORT); }

static unsigned short cursor_pos = 0;

void keyboard_handler(void) {
  unsigned char scan_code = read_scan_code();

  /* Ignora eventos de release (bit 7 ligado) */
  if (scan_code & 0x80) {
    return;
  }

  unsigned char ascii = scancode_to_ascii[scan_code];

  if (ascii != 0) {
    if (ascii == '\n') {
      /* Pula para o início da próxima linha (80 colunas por linha) */
      cursor_pos = cursor_pos + (80 - (cursor_pos % 80));
    } else if (ascii == '\b') {
      /* Backspace: volta uma posição e apaga o caractere */
      if (cursor_pos > 0) {
        cursor_pos--;
        fb_write_cell(cursor_pos * 2, ' ', 0, 0);
      }
    } else {
      unsigned int pos_char = cursor_pos * 2;
      fb_write_cell(pos_char, ascii, 10, 15);
      cursor_pos++;
    }

    fb_move_cursor(cursor_pos);
  }
}
