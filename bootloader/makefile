# Target
MCU = atmega32
# Clock Frequency
F_CPU = 16000000
# Baudrate to use
BAUDRATE = 38400
# Time in ms to wait for incoming files
BOOTDELAY = 750
# Command to delete files
RM = rm -rf

# ---------------------------------

OPT = s
CSTANDARD = gnu99

SRC = main.c
SRC += serial.c
SRC += parse.c
SRC += spm.c
SRC += util.c

SAMPLESRC = sample.c
SAMPLESRC += serial.c

OBJ = $(SRC:.c=.o)
SAMPLEOBJ = $(SAMPLESRC:.c=.o)

# Atmega 32:
# 256 Words: 3F00*2=0x7E00
# 512 Words: 3E00*2=0x7C00
# 1024 Words: 3C00*2=0x7800
# 2048 Words: 3800*2=0x7000
BOOTSTART = 0x7000

LINKER = -Wl,--relax
SECTION = ,-Ttext=$(BOOTSTART)

CARGS = -mmcu=$(MCU)
CARGS += -O$(OPT)
CARGS += -funsigned-char
CARGS += -funsigned-bitfields
CARGS += -fpack-struct
CARGS += -fshort-enums
CARGS += -Wall -Wstrict-prototypes
CARGS += -std=$(CSTANDARD)
CARGS += -DF_CPU=$(F_CPU)
CARGS += -DBOOTSTART=$(BOOTSTART)
CARGS += -DBAUDRATE=$(BAUDRATE)
CARGS += -DBOOTDELAY=$(BOOTDELAY)
CARGS += -ffreestanding
CARGS += --combine -fwhole-program

# ---------------------------------

all: yasab.hex sample.hex

size: yasab.hex
	make clean

program: yasab.hex
	avrdude -p $(MCU) -c stk500v2 -P /dev/tty.usbmodem641 -e -U yasab.hex
	make clean
	make sample.hex
	make cleanPart

dump:
	avrdude -p $(MCU) -c stk500v2 -P /dev/tty.usbmodem641 -U flash:r:flash.bin:r
	hexdump -C flash.bin

yasab.elf: $(SRC)
	avr-gcc $(CARGS) $(LINKER)$(SECTION) $(SRC) --output yasab.elf
	avr-size --mcu=$(MCU) -C yasab.elf

yasab.hex: yasab.elf
	avr-objcopy -O ihex yasab.elf yasab.hex
	avr-objdump -h -S yasab.elf > yasab.lss

sample.elf: $(SAMPLESRC)
	avr-gcc $(CARGS) $(LINKER) $(SAMPLESRC) --output sample.elf

sample.hex: sample.elf
	avr-objcopy -O ihex sample.elf sample.hex
	avr-objdump -h -S sample.elf > sample.lss

cleanPart:
	$(RM) *.o
	$(RM) *.lss
	$(RM) *.elf

clean: cleanPart
	$(RM) *.hex
	$(RM) *.bin
