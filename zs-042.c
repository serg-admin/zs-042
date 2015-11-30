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
  switch(result) {
      case TW_MT_DATA_ACK : 
        queue_putTask(DO_REQUEST_RTC_DATA_END);
        break;
      case TW_MR_DATA_NACK : //Результат получен
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

void commands_reciver(char* str) {
  uart_write("com=");
  uart_writeln(str);
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
        queue_putTask(DO_REQUEST_RTC_DATA_START);
        timer1_doing = 0;
        break;
      default : sleep_mode();
    }
  }
  return 0;
}
