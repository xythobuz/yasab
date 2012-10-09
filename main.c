/*
 * main.c
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of YASAB.
 *
 * YASAB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YASAB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with YASAB.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <avr/io.h>
#include <stdint.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "serial.h"

#define BAUDRATE 19200
#define BOOTDELAY 1000

#define XON() serialWrite(17)
#define XOFF() serialWrite(19)

#define PROGRAMMED() serialWrite('P');
#define PROGERROR() serialWrite('E');

#define WAITING 0
#define PARSING 1
#define EXIT 2

#define START 0
#define SIZE 1
#define ADDRESS 2
#define TYPE 3
#define DATA 4
#define CHECKSUM 5
#define ERROR 6

uint8_t appState = WAITING;
uint8_t buf[SPM_PAGESIZE];

void set(uint8_t *d, uint8_t c, uint16_t l);
void parse(uint8_t c);
uint16_t convert(uint8_t *d, uint8_t l);
void program(uint32_t page, uint8_t *d);
void gotoApplication(void);

int main(void) {
	uint8_t c;

	// Move Interrupt Vectors into Bootloader Section
	GICR = (1 << IVCE);
	GICR = (1 << IVSEL);

	serialInit(BAUD(BAUDRATE, F_CPU), 8, NONE, 1);
	sei();

	serialWriteString("Bootloader here\n");

	_delay_ms(BOOTDELAY);

	if (!serialHasChar()) {
		gotoApplication();
	}

	set(buf, 0xFF, sizeof(buf));
	serialWriteString("Send HEX File!\n");

	while(1) {
		if (appState == WAITING) {
			while(!serialHasChar());
			c = serialGet();
			if (c == ':') {
				appState = PARSING;
				parse(c);
			}
		} else if (appState == PARSING) {
			while(!serialHasChar());
			c = serialGet();
			parse(c);
		} else {
			serialWriteString("Thank you!\n");
			gotoApplication();
		}
	}

	return 0; // hehe...
}

void parse(uint8_t c) {
	static uint8_t parseState = START;
	static uint8_t hexBufI, size, nextPage = 1, hexCount, type;
	static uint16_t address, checksum, flashPage, flashI;
	static uint8_t hexBuf[5];

	if (parseState == START) {
		if (c == ':') {
			XOFF();
			parseState = SIZE;
			hexBufI = 0;
			checksum = 0;
			XON();
		}
	} else if (parseState == SIZE) {
		hexBuf[hexBufI++] = c;
		if (hexBufI >= 2) {
			XOFF();
			parseState = ADDRESS;
			hexBufI = 0;
			size = convert(hexBuf, 2);
			checksum += size;
			XON();
		}
	} else if (parseState == ADDRESS) {
		hexBuf[hexBufI++] = c;
		if (hexBufI >= 4) {
			XOFF();
			parseState = TYPE;
			hexBufI = 0;
			address = convert(hexBuf, 4);
			checksum += (address & 0xFF);
			checksum += ((address & 0xFF00) >> 8);
			if (nextPage) {
				flashPage = address - (address % SPM_PAGESIZE);
				nextPage = 0;
			}
			XON();
		}
	} else if (parseState == TYPE) {
		hexBuf[hexBufI++] = c;
		if (hexBufI >= 2) {
			XOFF();
			hexBufI = 0;
			hexCount = 0;
			type = convert(hexBuf, 2);
			checksum += type;
			if (type == 1) {
				parseState = CHECKSUM;
			} else {
				parseState = DATA;
			}
			XON();
		}
	} else if (parseState == DATA) {
		hexBuf[hexBufI++] = c;
		if (hexBufI >= 2) {
			XOFF();
			hexBufI = 0;
			buf[flashI] = convert(hexBuf, 2);
			checksum += buf[flashI];
			flashI++;
			hexCount++;
			if (hexCount == size) {
				parseState = CHECKSUM;
				hexCount = 0;
				hexBufI = 0;
			}
			if (flashI >= SPM_PAGESIZE) {
				// Write page into flash
				program(flashPage, buf);
				set(buf, 0xFF, sizeof(buf));
				flashI = 0;
				nextPage = 1;
			}
			XON();
		}
	} else if (parseState == CHECKSUM) {
		hexBuf[hexBufI++] = c;
		if (hexBufI >= 2) {
			XOFF();
			c = convert(hexBuf, 2);
			checksum += c;
			checksum &= 0x00FF;
			if (type == 1) {
				// EOF, write rest
				program(flashPage, buf);
				appState = EXIT;
			}
			if (checksum == 0) {
				parseState = START;
			} else {
				parseState = ERROR;
			}
			XON();
		}
	} else {
		PROGERROR();
		gotoApplication();
	}
}

void set(uint8_t *d, uint8_t c, uint16_t l) {
	uint16_t i;
	for (i = 0; i < l; i++) {
		d[i] = c;
	}
}

uint16_t convert(uint8_t *d, uint8_t l) {
	uint8_t i, c;
	uint16_t s = 0;

	for (i = 0; i < l; i++) {
		c = d[i];
		if ((c >= '0') && (c <= '9')) {
			c -= '0';
		} else if ((c >= 'A') && (c <= 'F')) {
			c -= 'A' - 10;
		} else if ((c >= 'a') && (c <= 'f')) {
			c -= 'a' - 10;
		}
		s = (16 * s) + c;
	}
	return s;
}

void program(uint32_t page, uint8_t *d) {
	uint16_t i, w;
	PROGRAMMED();
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		eeprom_busy_wait();
		boot_page_erase(page);
		boot_spm_busy_wait();
		for (i = 0; i < SPM_PAGESIZE; i += 2) {
			w = *d++;
			w += ((*d++) << 8);
			boot_page_fill(page + i, w);
		}
		boot_page_write(page);
		boot_spm_busy_wait();
		boot_rww_enable(); // Allows us to jump back
	}
}

void gotoApplication(void) {
	void (*realProgram)(void) = 0x0000; // Function Pointer to real program

	// Free Hardware Resources
	while(!transmitBufferEmpty());
	serialClose();

	cli();

	// Fix Interrupt Vectors
	GICR = (1 << IVCE);
	GICR = 0;

	// Call main program
#ifdef EIND
	EIND = 0; // Bug in gcc for Flash > 128KB
#endif
	realProgram();
}
