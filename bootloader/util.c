/*
 * spm.c
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
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "global.h"

void set(uint8_t *d, uint8_t c, uint16_t l) {
    uint16_t i;
    for (i = 0; i < l; i++) {
        d[i] = c;
    }
}

inline void gotoAddress(void (*app)(void), uint8_t c) {
    // Free Hardware Resources
    serialClose();
    cli();

    // Fix Interrupt Vectors
    uint8_t t = GICR;
    GICR = t | (1 << IVCE);
    GICR = t & ~(1 << IVSEL);

    // Call main program
#ifdef EIND
    EIND = c; // Bug in gcc for Flash > 128KB
#endif
    app();
}

typedef void (*StupidShit)(void);

void gotoApplication(void) {
    gotoAddress((StupidShit)0x0000, 0);
}
