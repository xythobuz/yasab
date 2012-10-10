/*
 * parse.c
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

#include "global.h"

void parse(uint8_t c) {
	static uint8_t parseState = START;
	static uint8_t hexBufI, size, nextPage = 1, hexCount, type;
	static uint16_t address, checksum, flashPage, flashI;
	static uint8_t hexBuf[5];
	static uint8_t appState = WAITING;
	static uint8_t buf[SPM_PAGESIZE];

	if (parseState == START) {
		if (c == ':') {
			XOFF();
			parseState = SIZE;
			hexBufI = 0;
			checksum = 0;
			set(buf, 0xFF, sizeof(buf));
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
