#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/twi.h>
#define ledOn PORTB |= _BV(PINB5)
#define ledOff PORTB &= ~(_BV(PINB5))
#define ledSw PORTB ^= _BV(PINB5)

//  0xD0 - for write, and 0xD1 for read
#define DS3231_ADDRESS 0xD0


#define FOSC 16000000 // Clock Speed
#define BAUD 38400
#define USART0_BUFER_SIZE 255
#include "tools/uart_async.h"

// Прерывание переполнения таймера
ISR (TIMER1_OVF_vect) {
  ledSw;
  init_i2c();
}

void timer_init() {
  // Делитель счетчика 256 (CS10=0, CS11=0, CS12=1).
  // 256 * 65536 = 16 777 216 (тактов)
  TCCR1B |= _BV(CS12);
  // Включить обработчик прерывания переполнения счетчика таймера.
  TIMSK1 = _BV(TOIE1);
  PRR &= ~(_BV(PRTIM1));

}

char get_hex_char(unsigned char c) {
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

void char_to_hex(unsigned char c) {
  char r[3];
  r[0] = get_hex_char((c & 0xF0) / 16);
  r[1] = get_hex_char(c & 0x0F) ;
  r[2] = 0;
  uart_writeln(r);
}

void ERROR(void) {
  uart_writeln("Error");
}

void init_i2c(void) {
  TWSR |= 3; // делитель частоты 64.
  uart_writeln("1");
  TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN); // Send START condition.
  while (!(TWCR & (1<<TWINT))); // Wait for TWINT Flag set. This indicates that the START condition has been transmitted.
  if ((TWSR & 0xF8) != TW_START) ERROR(); // Check value of TWI Status Register. Mask prescaler bits. If status different from START go to ERROR.
  uart_writeln("2");
  char_to_hex(TWSR);
  TWDR = DS3231_ADDRESS;
  TWCR = (1<<TWINT) | (1<<TWEN); // Load SLA_W into TWDR Register. Clear TWINT bit in TWCR to start transmission of address.
  while (!(TWCR & (1<<TWINT))); // Wait for TWINT Flag set. This indicates that the SLA_W has been transmitted, and ACK/NACK has been received.
  if ((TWSR & 0xF8) != TW_MT_SLA_ACK) ERROR(); // Check value of TWI Status Register. Mask prescaler bits. If status different from MT_SLA_ACK go to ERROR.
  char_to_hex(TWSR);
  if ((TWSR & 0xF8) == TW_MT_SLA_NACK) uart_writeln("NASK");
  if ((TWSR & 0xF8) == 0x38) uart_writeln("LOST");
  uart_writeln("3");
  TWDR = 0xFF;
  TWCR = (1<<TWINT) | (1<<TWEN); // Load DATA into TWDR Register. Clear TWINT bit in TWCR to start transmission of data.
  while (!(TWCR & (1<<TWINT))); // Wait for TWINT Flag set. This indicates that the DATA has been transmitted, and ACK/NACK has been received.
  if ((TWSR & 0xF8) != TW_MT_DATA_ACK) ERROR(); // Check value of TWI Status Register. Mask prescaler bits. If status different from MT_DATA_ACK go to ERROR.
  uart_writeln("4");
  TWCR = (1<<TWINT)|(1<<TWEN) | (1<<TWSTO); // Transmit STOP condition.
  uart_writeln("5");
}

int main(void) {
  // Разрешить светодиод arduino pro mini.
  DDRB |= _BV(DDB5);
  timer_init();
  uart_async_init();
  // Разрешить прерывания.
  sei();
  uart_writeln("start");
//  init_i2c();
  // Бесконечный цыкл с энергосбережением.
  for(;;) sleep_mode();

  return 0;
}
