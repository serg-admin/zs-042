/*
   Содержит функции для асинхронной записи в UART.
   Перед использованием необходимо установить:
     FOSC  - Тактовая частота
     BAUND - Желаемая скорость порта. На некоторых частотах возможно возникновении 
             погрешности несущей частоты что делает невозможной работу порта.
             Смотрите документацию на микропроцессор
*/
#ifndef FOSC
  #define FOSC 16000000 // Clock Speed
#endif /* END FOSC */

#ifndef BAUD
  #define BAUD 38400
#endif /* END BAUD */
#define MYBDIV (FOSC / 16 / BAUD - 1)

#ifndef USART0_BUFER_SIZE
  #define UART0_BUFER_SIZE 32
#endif /* UART0_BUFER_SIZE */

//Отправляет один байт в очередь USART. В случае если очередь занята - ждет.
void uart_putChar(char c);

//Отправляет 0 терменированную строку в очередь USART.
void uart_write(char* s);

//Отправляет строку в очередь USART. В случае если очередь занята - ждет.
void uart_writeln(char* s);

void uart_async_init(void);