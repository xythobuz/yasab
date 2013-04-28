# Copyright (c) 2012, Thomas Buck <xythobuz@me.com>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Target
MCU = atmega2560
# Clock Frequency
F_CPU = 16000000
# Baudrate to use
BAUDRATE = 38400
# Time in ms to wait for incoming files
BOOTDELAY = 1500
# Command to delete files
RM = rm -rf

ISP = usb
TYPE = avrisp2
TARGET = /dev/tty.usbserial-AE01539L
#TARGET = /dev/tty.usbserial-A100QOUE

# ---------------------------------

OPT = s
CSTANDARD = gnu99

SRC = main.c
SRC += serial.c
SRC += spm.c
SRC += util.c

SAMPLESRC = sample.c
SAMPLESRC += serial.c

OBJ = $(SRC:.c=.o)
SAMPLEOBJ = $(SAMPLESRC:.c=.o)

# Atmega 32:
# --> 1024 Words: 3C00*2=0x7800
# Atmega 2560:
# --> 2048 Words: 1F800*2=0x3F000
# For other MCUs, calculate using
# "Boot Size Configuration" Table in Datasheet
BOOTSTART = 0x3F000

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

test: sample.hex
	yasab $(TARGET) sample.hex q

program: yasab.hex
	avrdude -V -p $(MCU) -c $(TYPE) -P $(ISP) -e -U yasab.hex
	make clean
	make sample.hex
	make cleanPart

programSample: sample.hex
	avrdude -V -p $(MCU) -c $(TYPE) -P $(ISP) -e -U sample.hex
	make clean

dump:
	avrdude -V -p $(MCU) -c $(TYPE) -P $(ISP) -U flash:r:flash.bin:r
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
