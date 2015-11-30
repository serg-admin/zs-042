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

#define DO_TIMER1_OVF 0x01
#define DO_INT0 0x02
#define DO_INT1 0x03
#define DO_REQUEST_RTC_DATA_START 0x20
#define DO_REQUEST_RTC_DATA_END 0x21
unsigned char queue_tasks_current;

byte decToBcd(byte val){
  return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val){
  return ( (val/16*10) + (val%16) );
}

// Очередь задачь
#define QUEUE_TASKS_SIZE 10
unsigned char queue_tasks[QUEUE_TASKS_SIZE];
unsigned char queue_tasks_rpos;
unsigned char queue_tasks_wpos;

void queue_init(void) {
  queue_tasks_rpos = 0;
  queue_tasks_wpos = 0;
  queue_tasks_current = 0;
}

unsigned char queue_getTask() {
  if (queue_tasks_rpos == queue_tasks_wpos) return 0;
  queue_tasks_current = queue_tasks[queue_tasks_rpos++];
  if (queue_tasks_rpos >= QUEUE_TASKS_SIZE) queue_tasks_rpos = 0;
  return queue_tasks_current;
}

void queue_putTask(unsigned char task) {
  cli();
  queue_tasks[queue_tasks_wpos++] = task;
  if (queue_tasks_wpos >= QUEUE_TASKS_SIZE) queue_tasks_wpos = 0;
  sei();
}

void callBackForRequestRtcData(unsigned char result) {
  byte i = 0x0E;
  switch(result) {
      case TW_MT_DATA_ACK : 
        //uart_writeln("SEND OK");
        queue_putTask(DO_REQUEST_RTC_DATA_END);
        break;
      case TW_MR_DATA_NACK :
        //uart_writeln("RECIVE OK");
        for(i = 0x0E; i < 16; i++) {
          //uart_writelnHEX(ds3231_buf[i]);
        }
       /* uart_writelnHEX(ds3231_buf[0]);
        uart_writelnHEX(ds3231_buf[1]);
        uart_writelnHEX(ds3231_buf[2]);
        uart_writelnHEX(ds3231_buf[3]);
        uart_writelnHEX(ds3231_buf[4]);
        uart_writelnHEX(ds3231_buf[5]);
        uart_writelnHEX(ds3231_buf[6]);*/
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

ISR (PCINT1_vect) {
  cli();
  queue_putTask(DO_INT1);
  sei();
}

ISR (PCINT0_vect) {
  cli();
  //queue_putTask(DO_INT0);
  if (PORTB  & _BV(PINB5)) {
  unsigned char h = TCNT1H;
  unsigned char l = TCNT1L;
  //TCNT1 = 0;
  uart_writeln("_");
  uart_writelnHEX(h);
  uart_writelnHEX(l);
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
  timer1_doing = 0; // Задача таймера не по

}

void zs042_setTime(byte second, byte minuts, byte hour, byte deyOfWeek, byte deyOfMonth, byte month, byte year) {
  ds3231_buf[0] = 0;
  ds3231_buf[1] = 0x10; //second
  ds3231_buf[2] = 0x17; //minut
  ds3231_buf[3] = 0x12; //hour
  ds3231_buf[4] = 0x07; //dey of week
  ds3231_buf[5] = 0x29; //dey of month
  ds3231_buf[6] = 0x11; //month
  ds3231_buf[7] = 0x15; //year
  i2c_send(0xD0, ds3231_buf, 8, &callBackForRequestRtcData);
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
 // DDRB &= ~(_BV(DDB1));
 // DDRD |= _BV(DDD3);
 // ledOn;
  timer_init();
  uart_async_init();
  //i2c_init();
  queue_init();
  //EIMSK = 2;
 /* PCMSK2 |= _BV(PCINT19);
  PCMSK0 |= _BV(PCINT5);
  PCMSK0 |= _BV(PCINT1);
  PORTB |= _BV(PINB5); 
  PCICR |= _BV(PCIE0);
  PCICR |= _BV(PCIE1);
  PCICR |= _BV(PCIE2);*/
  // Разрешить прерывания.
  sei();
  
  // Импульсы  1HZ
 // ds3231_buf[0] = 0x0E; 
 // ds3231_buf[1] = 0x00; //second
  //i2c_send(0xD0, ds3231_buf, 2, &stub);
  
  uart_readln(&commands_reciver);
  //uart_writeln("START");
  // Бесконечный цикл с энергосбережением.
  for(;;) {
    switch(queue_getTask()) {
      case DO_REQUEST_RTC_DATA_START : 
        ds3231_buf[0] = 0;
        //i2c_send(0xD0, ds3231_buf, 1, &callBackForRequestRtcData);
        break;
      case DO_REQUEST_RTC_DATA_END :
        //i2c_recive(0xD0 + 1, ds3231_buf, 16, &callBackForRequestRtcData);
        break;
      case DO_TIMER1_OVF :
        //uart_writeln("OVF");
        i2c_init();
        queue_putTask(DO_REQUEST_RTC_DATA_START);
        timer1_doing = 0;
        break;
      case DO_INT0 :
        uart_writeln("DO_INT0");
        break;
      case DO_INT1 :
        uart_writeln("DO_INT1");
        break;  
      default : sleep_mode();
    }
  }

  return 0;
}
