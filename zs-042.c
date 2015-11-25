#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#define ledOn PORTB |= _BV(PINB5)
#define ledOff PORTB &= ~(_BV(PINB5))
#define ledSw PORTB ^= _BV(PINB5)

// Прерывание переполнения таймера
ISR (TIMER1_OVF_vect) {
  ledSw;
}

int
main(void) {
  // Разрешить светодиод arduino pro mini.
  DDRB |= _BV(DDB5);
  // Делитель счетчика 256 (CS10=0, CS11=0, CS12=1).
  // 256 * 65536 = 16 777 216 (тактов)
  TCCR1B |= _BV(CS12);
  // Включить обработчик прерывания переполнения счетчика таймера.
  TIMSK1 = _BV(TOIE1);
  PRR &= ~(_BV(PRTIM1));
  // Разрешить прерывания.
  sei();
  // Бесконечный с энергосбережением.
  for(;;) sleep_mode();

  return 0;
}
