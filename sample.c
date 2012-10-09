/*
 * sample.c
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
#include <util/delay.h>
#include <avr/interrupt.h>

#include "serial.h"

#define BAUDRATE 38400

int main(void) {
	serialInit(BAUD(BAUDRATE, F_CPU), 8, NONE, 1);
	sei();

	DDRA = 0xC0;
	PORTA |= 0x40;

	while(1) {
		serialWriteString("Hi there...\n");
		PORTA ^= 0xC0;
		_delay_ms(500);
		if (serialHasChar()) {
			serialWriteString("UUhh... He sent '");
			serialWrite(serialGet());
			serialWriteString("'...\n");
		}
	}
	return 0;
}
