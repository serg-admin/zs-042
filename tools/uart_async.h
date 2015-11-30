/*
   Содержит функции для асинхронной записи в UART.
   Перед использованием необходимо установить:
     FOSC  - Тактовая частота
     BAUND - Желаемая скорость порта. На некоторых частотах возможно возникновении 
             погрешности несущей частоты что делает невозможной работу порта.
             Смотрите документацию на микропроцессор
*/

#define FOSC 16000000 // Clock Speed

//#define BAUD 2400
#define BAUD 38400
//#define BAUD 57600
//#define BAUD 115200

#define MYBDIV (FOSC / 16 / BAUD - 1)
#define UART0_BUFER_SIZE 32
#define UART0_READ_BUFER_SIZE 32

//Отправляет один байт в очередь USART. В случае если очередь занята - ждет.
void uart_putChar(char c);

//Отправляет 0 терменированную строку в очередь USART.
void uart_write(char* s);

//Отправляет строку в очередь USART. В случае если очередь занята - ждет.
void uart_writeln(char* s);

void uart_async_init(void);

//Выводит в порт один байт в ввиде HEX строки - 0xXX.
void uart_writelnHEX(unsigned char c);

// Вычитывает с порта строку до окончания строки, и возвращает в callBack функцию.
// Строка завершается нулем. CallBack функция будет вызываться для каждой полученной строки.
void uart_readln(void (*callback)(char*));