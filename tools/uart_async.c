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
    UDR0 = uart_buf[uart_rPos];
    uart_rPos++;
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
  if (UCSR0A & (1<<UDRE0)) {
    UDR0 = c;
    return;
  }
  if (uart_wPos != uart_rPos)
  while(uart_getBufSpace() == 0) sleep_mode();
  uart_buf[uart_wPos++] = c;
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