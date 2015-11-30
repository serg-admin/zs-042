/*
 *  Программа для микрокрнтроллера AVR ATMEGA328P (Arduino pro mini). 
 *  Для тестирования и насройки чипа RTC DS3231 или модуля ZS-042. 
 *  Шеснадцатиричные команды отправляются на USART порт.
 *  Пример команд:
 *  
 *  Установка времени:
 *    xD00001171301301115 - Установить время дату:
 *      x  - префикс шеснадцатиричной команды;
 *      D0 - номер I2C устройства для записи данных;
 *      00 - адрес ячейки памяти с который будем писать;
 *      01 - секунды;
 *      17 - минуты;
 *      13 - часы;
 *      01 - день недели (понедельник);
 *      30 - число;
 *      11 - месяц (ноябрь);
 *      15 - год
 *    Будет установлена дата: 30 ноября 15-го года 13:17:01
 * 
 *  Получение температуры чипа:
 *    Запрашиваем 0x11 регистр данных:
 *      xD011 
 *        x  - префикс шеснадцатиричной команды;
 *        D0 - номер I2C устройства для записи данных;
 *        11 - адрес регистра данных;
 *    Читаем два байта от запрошенного адреса:
 *      xD102
 *        x  - префикс шеснадцатиричной команды;
 *        D1 - номер I2C устройства для записи данных;
 *        02 - длина записи в байтах
 *    Результат:
 *      OK   - подтверждение отправки первой команды
 *      0x1C - 28 градусов цельсия
 *      0x80 - еще 0.5 градусов цельсия, и того 28.5 градусов.
 * 
 *   Регистры чипа DS3231 (шоб не искать каждый раз).
 *   АДРЕС  BIT 7    BIT 6   BIT 5   BIT 4   BIT 3   BIT 2   BIT 1   BIT 0    ФУНКЦИЯ    ДИАПАЗОН
 *           MSB                                                      LSB
 *   0x00     0    |   10-ки секунд        |        едениницы секунд        | Секунды     00-59
 *   0x01     0    |   10-ки минут         |        еденицы минут           | Минуты
 *   0x02     0    |12/!24 | !AM/PM|  0-4  |        еденицы часов           | Часы        1-12 + !AM/PM
 *                 |       |  0-2  |       |                                |             00-23
 *   0x03     0    |   0   |   0   |   0   |   0   | День недели            | День недели 1-7  
 *   0x04     0    |   0   |      0-3      |       | День месяца            | День месяца 01-31
 *   0x05 Век(пере-|   0   |   0   |  0-1  |          Месяц                 | Месяц/пере- 01-12 + 
 *        полнение                         |                                | ход века     век(128)
 *        годов)                           |
 *   0x06         10-ки лет
 *   0x07    A1M1  |   10-ки секунд        |        едениницы секунд        | Будильник 1 секунды 00-59
 *   0x08    A1M2  |   10-ки минут         |        едениницы минут         | Будильник 1 минуты  00-59
 *   0x09    A1M3  |12/!24 | !AM/PM|  0-4  |        еденицы часов           | Будильник 1 часы    1-12 + !AM/PM
 *                 |       |  0-2  |       |                                |             00-23
 *   0x0A    A1M4  |Еженед.|    десятки    |        День недели             | Будильник 1 день  1-7         
 *                  Ежемес.|               |        День месяца             |                   1-31
 *   0x0B    A2M2  |   10-ки минут         |        едениницы минут         | Будильник 1 минуты  00-59
 *   0x0C    A3M3  |12/!24 | !AM/PM|  0-4  |        еденицы часов           | Будильник 1 часы    1-12 + !AM/PM
 *                 |       |  0-2  |       |                                |             00-23
 *   0x0D    A4M4  |Еженед.|    десятки    |        День недели             | Будильник 1 день  1-7         
 *                 |Ежемес.|               |        День месяца             |                   1-31
 *   0x0E   !EOSC  |BBSQW  | CONV  | RS2   | RS1   | INTCN |   A2IE | A1IE  | Контрольный регистр
 *   0x0F    OSF   |   0   |   0   |   0   |EN32kHz|  BSY  |  A2F   | A1F   | Контрольный/Статусы
 *   0x10    ЗНАК  | DATA  | DATA  | DATA  | DATA  | DATA  | DATA   | DATA  | Корректировка хода
 *   0x11    ЗНАК  | DATA  | DATA  | DATA  | DATA  | DATA  | DATA   | DATA  | Целые температуры
 *   0x12    DATA  | DATA  |   0   |   0   |   0   |   0   |   0    |   0   | Дробная часть температуры
 *                                                                            0.0,0.25,0.5,0.75
 * 
 *  *Настройки переодичности будильников в документации к чипу
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define byte unsigned char

//Настройки магающей лампочки
#define ledOn PORTB |= _BV(PINB5)  
#define ledOff PORTB &= ~(_BV(PINB5))
#define ledSw PORTB ^= _BV(PINB5)

//  0xD0 - for write, and 0xD1 for read
#define DS3231_ADDRESS 0xD0
#define DS3231_BUF_SIZE 16
#define HEX_CMD_MAX_SIZE 16
unsigned char ds3231_buf[DS3231_BUF_SIZE];

#include "tools/i2c_async.h"
#include "tools/uart_async.h"
#include "tools/queue_tasks.h"

byte decToBcd(byte val){
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val){
  return ( (val/16*10) + (val%16) );
}

void callBackForRequestRtcData(unsigned char result) {
  byte i;
  switch(result) {
    case TW_MT_DATA_ACK : 
      queue_putTask(DO_REQUEST_RTC_DATA_END);
      break;
    case TW_MR_DATA_NACK : //Результат получен
      for(i = 0; i < 8; i++) {
        uart_writelnHEX(ds3231_buf[i]);
      }
      break;
    default : 
      uart_write("ERROR ");
      uart_writelnHEX(result);
  }
}

// Прерывание переполнения таймера
unsigned char timer1_doing; // 0 - задачи таймера нет в очереди
ISR (TIMER1_OVF_vect) {
  cli();
  ledSw;
  if (timer1_doing == 0) {
    queue_putTask(DO_TIMER1_OVF);
    timer1_doing = 1;
  }
  sei();
}

ISR (BADISR_vect) {
  cli();
  TWSR = 0;
  sei();
}

void timer_init() {
  // Делитель счетчика 256 (CS10=0, CS11=0, CS12=1).
  // 256 * 65536 = 16 777 216 (тактов)
  TCCR1B |= _BV(CS12);
  //TCCR1B |= _BV(CS10); //Включить для мигания  4 -ре секунды
  // Включить обработчик прерывания переполнения счетчика таймера.
  TIMSK1 = _BV(TOIE1);
  // PRR &= ~(_BV(PRTIM1));
  timer1_doing = 0; // Задача таймера не активна
}

void zs042_setTime(byte second, byte minuts, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year) {
  ds3231_buf[0] = 0;
  ds3231_buf[1] = second;    //second
  ds3231_buf[2] = minuts;    //minut
  ds3231_buf[3] = hour;      //hour
  ds3231_buf[4] = dayOfWeek; //dey of week
  ds3231_buf[5] = dayOfMonth;//day of month
  ds3231_buf[6] = month;     //month
  ds3231_buf[7] = year;      //year
  i2c_send(0xD0, ds3231_buf, 8, &callBackForRequestRtcData);
}

void zs042_set1Hz(void) {
  ds3231_buf[0] = 0x0E; 
  // Импульсы  1HZ
  ds3231_buf[1] = 0x00;
  i2c_send(0xD0, ds3231_buf, 2, 0);  
}

unsigned char hexToCharOne(char c) {
  switch(c) {
    case '0' : return 0x00;
    case '1' : return 0x01;
    case '2' : return 0x02;
    case '3' : return 0x03;
    case '4' : return 0x04;
    case '5' : return 0x05;
    case '6' : return 0x06;
    case '7' : return 0x07;
    case '8' : return 0x08;
    case '9' : return 0x09;
    case 'A' : return 0x0A;
    case 'B' : return 0x0B;
    case 'C' : return 0x0C;
    case 'D' : return 0x0D;
    case 'E' : return 0x0E;
    case 'F' : return 0x0F;
    default : return 0xFF;
  }
}

byte cmd[HEX_CMD_MAX_SIZE];
byte* commands_reciver_param1;
byte* commands_reciver_param2;
byte* commands_reciver_param3;
#define COMMAND_SEND_I2C 0x30
#define COMMAND_RECIVE_I2C 0x31
#define COMMAND_GET_DATE 0x32

void callBackForSendI2CData(unsigned char result) {
  switch(result) {
    case TW_MT_DATA_ACK : 
      uart_writeln("OK");
      break;
    default : 
      uart_write("ERROR ");
      uart_writelnHEX(result);
  }
}

void callBackForReciveI2CData(unsigned char result) {
  byte i;
  switch(result) {
      case TW_MR_DATA_NACK : //Результат получен
        for(i = 0; i<commands_reciver_param3[0]; i++) {
          uart_writelnHEX(ds3231_buf[i]);
        }
        break;
      default : 
        uart_write("ERROR ");
        uart_writelnHEX(result);
  }
}

void commands_reciver(char* str) {
  byte pos = 0;
  byte tmp;
  if (str[0] == 'x') {
    while((str[pos*2+1] != 0) && 
          (pos < (HEX_CMD_MAX_SIZE-2/*два байта для вычисляемых параметров*/))) {
      cmd[pos] = hexToCharOne(str[pos*2+1]);
      tmp = hexToCharOne(str[pos*2+2]);
      if ((tmp > 0xF) || (cmd[pos] > 0xF)) {
        uart_writeln("PARS ERR");
        return;
      }
      cmd[pos] <<= 4;
      cmd[pos] |= tmp;
      pos++;
    }
    switch (cmd[0]) {
      case DS3231_ADDRESS : // отправить на RTC zs-042
        commands_reciver_param1 = cmd; //Адрес устройства
        commands_reciver_param2 = (cmd+1); //Массив для отправки
        cmd[HEX_CMD_MAX_SIZE-1] = pos - 1;
        commands_reciver_param3 = (cmd + HEX_CMD_MAX_SIZE-1); //Количество отправляемых байт
        queue_putTask(COMMAND_SEND_I2C);
        break;
      case DS3231_ADDRESS+1 : // прочитать с RTC zs-042
        commands_reciver_param1 = cmd; //Адрес устройства
        commands_reciver_param3 = (cmd+1); //Количество получаемых байт
        queue_putTask(COMMAND_RECIVE_I2C);
        break;
      case COMMAND_GET_DATE :
        //queue_putTask(DO_REQUEST_RTC_DATA_START);
        break;
      default : 
        uart_write("UNKNOW COMM "); uart_writelnHEX(cmd[0]);
    }
  }
}

void stub(unsigned char result) {
  uart_write("==");
  uart_writelnHEX(result);
}

int main(void) {

  // Разрешить светодиод arduino pro mini.
  DDRB |= _BV(DDB5);
  timer_init();
  uart_async_init();
  i2c_init();
  queue_init();
  // Разрешить прерывания.
  sei();
  
  uart_readln(&commands_reciver);
  // Бесконечный цикл с энергосбережением.
  for(;;) {
    switch(queue_getTask()) {
      case DO_REQUEST_RTC_DATA_START : //Установть позицию на регистр 0x0
        ds3231_buf[0] = 0;
        i2c_send(0xD0, ds3231_buf, 1, &callBackForRequestRtcData);
        break;
      case DO_REQUEST_RTC_DATA_END : //Читаем 7 байт
        i2c_recive(0xD0 + 1, ds3231_buf, 7, &callBackForRequestRtcData);
        break;
      case DO_TIMER1_OVF :
        i2c_init();
        //queue_putTask(DO_REQUEST_RTC_DATA_START);
        timer1_doing = 0;
        break;
      case COMMAND_SEND_I2C :
        i2c_send(commands_reciver_param1[0], // Адресс.
                 commands_reciver_param2, // Буфер.
                 commands_reciver_param3[0], // Количество.
                 &callBackForSendI2CData);
        break;
      case COMMAND_RECIVE_I2C :
        i2c_recive(commands_reciver_param1[0], // Адресс.
                 ds3231_buf, // Буфер.
                 commands_reciver_param3[0], // Количество.
                 &callBackForReciveI2CData);
        break;
      default : sleep_mode();
    }
  }
  return 0;
}
