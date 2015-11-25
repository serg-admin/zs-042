PRG            = zs-042
OBJ            = $(PRG).o
#MCU_TARGET     = at90s2313
#MCU_TARGET     = at90s2333
#MCU_TARGET     = at90s4414
#MCU_TARGET     = at90s4433
#MCU_TARGET     = at90s4434
#MCU_TARGET     = at90s8515
#MCU_TARGET     = at90s8535
#MCU_TARGET     = atmega128
#MCU_TARGET     = atmega1280
#MCU_TARGET     = atmega1281
#MCU_TARGET     = atmega1284p
#MCU_TARGET     = atmega16
#MCU_TARGET     = atmega163
#MCU_TARGET     = atmega164p
#MCU_TARGET     = atmega165
#MCU_TARGET     = atmega165p
#MCU_TARGET     = atmega168
#MCU_TARGET     = atmega169
#MCU_TARGET     = atmega169p
#MCU_TARGET     = atmega2560
#MCU_TARGET     = atmega2561
#MCU_TARGET     = atmega32
#MCU_TARGET     = atmega324p
MCU_TARGET     = atmega328p
#MCU_TARGET     = atmega325
#MCU_TARGET     = atmega3250
#MCU_TARGET     = atmega329
#MCU_TARGET     = atmega3290
#MCU_TARGET     = atmega32u4
#MCU_TARGET     = atmega48
#MCU_TARGET     = atmega64
#MCU_TARGET     = atmega640
#MCU_TARGET     = atmega644
#MCU_TARGET     = atmega644p
#MCU_TARGET     = atmega645
#MCU_TARGET     = atmega6450
#MCU_TARGET     = atmega649
#MCU_TARGET     = atmega6490
#MCU_TARGET     = atmega8
#MCU_TARGET     = atmega8515
#MCU_TARGET     = atmega8535
#MCU_TARGET     = atmega88
#MCU_TARGET     = attiny2313
#MCU_TARGET     = attiny24
#MCU_TARGET     = attiny25
#MCU_TARGET     = attiny26
#MCU_TARGET     = attiny261
#MCU_TARGET     = attiny44
#MCU_TARGET     = attiny45
#MCU_TARGET     = attiny461
#MCU_TARGET     = attiny84
#MCU_TARGET     = attiny85
#MCU_TARGET     = attiny861
OPTIMIZE       = -O2

# You should not have to change anything below here.

CC             = avr-gcc

DEFS           = -I /usr/lib/avr/include/
LIBS           =

# Override is only needed by avr-lib build system.

override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS       = -Wl,-Map,$(PRG).map

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump
#-------------------
all: $(PRG).hex
#-------------------
$(PRG).hex : $(PRG).out
	$(OBJCOPY) -R .eeprom -O ihex $(PRG).out $(PRG).hex
$(PRG).out : $(OBJ)
	$(CC) $(CFLAGS) -o $(PRG).out $(LDFLAGS) $(OBJ)
$(OBG) : $(PRG).c
	$(CC) $(CFLAGS) -Os -c $(PRG).c
pro_mini: $(PRG).hex
	avrdude -c arduino -F -P /dev/ttyUSB0 -p $(MCU_TARGET) -b 57600  -U flash:w:$(PRG).hex:i
leonardo: $(PRG).hex
	avrdude -c avr109 -F -P /dev/ttyACM1 -p m32u4 -U flash:w:$(PRG).hex:i
#-------------------
clean:
	rm -f *.o *.map *.out *.lst *.hex
#-------------------
get_asm: $(PRG).c
	$(CC) $(CFLAGS) -o $(PRG).elf $(OBJ)
	$(OBJDUMP) -h -S $(PRG).elf > $(PRG).lst
#-------------------