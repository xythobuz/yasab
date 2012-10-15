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

#define DEBUG 1
#define VERSION "1.0"

#include "global.h"

uint8_t appState = WAITING;

void main(void) __attribute__ ((noreturn));
void main(void) {
    uint8_t c;

    // Move Interrupt Vectors into Bootloader Section
    c = GICR;
    GICR = c | (1 << IVCE);
    GICR = c | (1 << IVSEL);

    serialInit(BAUD(BAUDRATE, F_CPU));
    sei();

    debugPrint("YASAB ");
    debugPrint(VERSION);
    debugPrint(" by xythobuz\n");

    _delay_ms(BOOTDELAY);

    if (!serialHasChar()) {
        gotoApplication();
    }

    for(;;) {
        if (appState == PARSING) {
            while(!serialHasChar());
            c = serialGet();
            parse(c);
        } else if (appState == WAITING) {
            while(!serialHasChar());
            c = serialGet();
            if (c == ':') {
                appState = PARSING;
                parse(c);
            } else {
                serialWriteString(CONNECTED);
            }
        } else {
            serialWriteString(OKAY);
            gotoApplication();
        }
    }
}
