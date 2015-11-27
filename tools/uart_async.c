#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "uart_async.h"

static unsigned char uart_buf[UART0_BUFER_SIZE];
static unsigned char uart_wPos = 0; // Позиция буфера для записи новых данных.
static unsigned char uart_rPos = 0; // Позиция буфера для передачи в порт.

// Прерывание - завершение передачи.
ISR (USART_TX_vect) {
  cli();
  if (uart_wPos != uart_rPos) {
    UDR0 = uart_buf[uart_rPos++];
    if (uart_rPos >= UART0_BUFER_SIZE) uart_rPos = 0;
  }
  sei();
}

void uart_async_init(void) {
   // Разрешить передачу через порт.
   UCSR0B |= _BV(TXEN0);
   // Активировать прерывание по окончанию передачи.
   UCSR0B |= _BV(TXCIE0);
   // Устанавливаем скорость порта.
   UBRR0 = MYBDIV;
}

// Возвращает количество свободных байт в очереди USART.
char uart_getBufSpace() {
  if (uart_rPos <= uart_wPos) {
    return UART0_BUFER_SIZE - uart_wPos + uart_rPos;
  } else {
    return uart_rPos - uart_wPos - 1;
  }
}

// Отправляет один байт в очередь USART. В случае если очередь занята - ждет.
void uart_putChar(char c) {
/*  if ((UCSR0A & (1<<UDRE0)) && (uart_wPos == uart_rPos)) {
    UDR0 = c;
    return;
  }*/
  cli();
  while(uart_getBufSpace() == 0) sleep_mode();
  uart_buf[uart_wPos++] = c;
  if (uart_wPos >= UART0_BUFER_SIZE) uart_wPos = 0;
  if (UCSR0A & _BV(UDRE0)){
    if (uart_wPos != uart_rPos) {
      UDR0 = uart_buf[uart_rPos++];
      if (uart_rPos >= UART0_BUFER_SIZE) uart_rPos = 0;
    }
  }
  sei();
}

// Отправляет 0 терменированную строку в очередь USART.
void uart_write(char* s) {
  unsigned char i = 0;
  while(s[i] != 0) {
    uart_putChar(s[i++]);
  }
}

// Отправляет строку в очередь USART. В случае если очередь занята - ждет.
void uart_writeln(char* s) {
    uart_write(s);
    uart_write("\n\r");
}

char uart_halfchar_to_hex(unsigned char c) {
  switch (c) {
    case 0 : return '0';
    case 1 : return '1';
    case 2 : return '2';
    case 3 : return '3';
    case 4 : return '4';
    case 5 : return '5';
    case 6 : return '6';
    case 7 : return '7';
    case 8 : return '8';
    case 9 : return '9';
    case 10 : return 'A';
    case 11 : return 'B';
    case 12 : return 'C';
    case 13 : return 'D';
    case 14 : return 'E';
    case 15 : return 'F';
  }
  return 0;
}

void uart_writelnHEX(unsigned char c) {
  char r[5];
  r[0] = '0';
  r[1] = 'x';
  r[2] = uart_halfchar_to_hex((c & 0xF0) / 16);
  r[3] = uart_halfchar_to_hex(c & 0x0F) ;
  r[4] = 0;
  uart_writeln(r);
}