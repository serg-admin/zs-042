#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/twi.h>
#define ledOn PORTB |= _BV(PINB5)
#define ledOff PORTB &= ~(_BV(PINB5))
#define ledSw PORTB ^= _BV(PINB5)

//  0xD0 - for write, and 0xD1 for read
#define DS3231_ADDRESS 0xD0
#define I2C_STATE_FREE 0x00
#define I2C_STATE_SEND 0x01


#define FOSC 16000000 // Clock Speed
#define BAUD 38400
#define USART0_BUFER_SIZE 16
#include "tools/uart_async.h"

void ERROR(void) {
  uart_writeln("Error");
}

unsigned char i2c_state;
void i2c_init(void) {
  TWBR = 0x10; //Делитель = TWBR * 2.
  TWCR |= _BV(TWIE); //Включить прерывание.
  i2c_state = I2C_STATE_FREE;
}

void next(void) {
  uart_writeln("STOP");
}

char i2c_send_dev_addr;
unsigned char* i2c_send_buf;
unsigned char i2c_send_buf_pos;
unsigned char i2c_send_size;
void (*i2c_callback)(void);


unsigned char i2c_send(char addr, unsigned char* buf, unsigned char size, void (*callback)(void)) {
  if (i2c_state) return i2c_state;
//  uart_writeln("START TWI");
  i2c_send_dev_addr = addr;
  i2c_send_buf = buf;
  i2c_send_buf_pos = 0;
  i2c_send_size = size;
  i2c_callback = callback;
  i2c_state = I2C_STATE_SEND;
  TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN) | _BV(TWIE); // Send START condition.
  return I2C_STATE_FREE;
}

void i2c_send_isp(unsigned char state) {
  uart_write("ISP state = ");
  uart_writelnHEX(state);
  switch(state) {
    case TW_START : //Шина I2C переведена в состояние start transmission. Запрашиваем устройство.
      TWDR = i2c_send_dev_addr;
      TWCR = (1<<TWINT) | (1<<TWEN) | _BV(TWIE); // Load SLA+W into TWDR Register. Clear TWINT bit in TWCR to start transmission of address.
      break;
    case TW_MT_SLA_ACK : // Устройство ответело, ожидает данные.
      TWDR = i2c_send_buf[i2c_send_buf_pos++];
      TWCR = (1<<TWINT) | (1<<TWEN) | _BV(TWIE);
      break;
    case TW_MT_DATA_ACK : // Данные успешно переданы.
      if (i2c_send_buf_pos < i2c_send_size) {
        TWDR = i2c_send_buf[i2c_send_buf_pos++];
        TWCR = (1<<TWINT) | (1<<TWEN) | _BV(TWIE);
      } else {
        uart_writeln("STOP TWI");
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWSTO);// | _BV(TWIE); // Остановка передача
        i2c_state = I2C_STATE_FREE;
        i2c_callback();
      }
      break;
    default : TWCR = 0;
  }
}

ISR (TWI_vect) {
  cli();
  TWCR = 0;
  uart_write("i2c_state = ");
  uart_writelnHEX(i2c_state);
  switch (i2c_state) {
    case I2C_STATE_SEND : i2c_send_isp(TWSR & 0xF8); break;
  }
  sei();
}

// Прерывание переполнения таймера
ISR (TIMER1_OVF_vect) {
  cli();
  ledSw;
//  init_i2c();
  sei();
}

ISR (BADISR_vect) {
  cli();
//  uart_writeln("BAD ISP");
  sei();
}

void timer_init() {
  // Делитель счетчика 256 (CS10=0, CS11=0, CS12=1).
  // 256 * 65536 = 16 777 216 (тактов)
  TCCR1B |= _BV(CS12);
  TCCR1B |= _BV(CS10);
  // Включить обработчик прерывания переполнения счетчика таймера.
  TIMSK1 = _BV(TOIE1);
  // PRR &= ~(_BV(PRTIM1));

}

int main(void) {
  // Разрешить светодиод arduino pro mini.
  unsigned char data[1];
  data[0] = 0;
  DDRB |= _BV(DDB5);
  timer_init();
  uart_async_init();
  // Разрешить прерывания.
  sei();
  uart_writeln("start");
  i2c_init();
  i2c_send(0xD0, data, 1, &next);

  // Бесконечный цыкл с энергосбережением.
  for(;;) sleep_mode();

  return 0;
}
