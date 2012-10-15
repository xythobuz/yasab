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
#include <avr/boot.h>

#define DEBUG 0

#include "global.h"

#if DEBUG >= 1
#include <stdlib.h>
char buff[5];
#endif

void program(uint32_t page, uint8_t *d) {
    uint16_t i;

#if DEBUG >= 1
    debugPrint("\nProgramming 0x");
    debugPrint(ultoa(page, buff, 16));
    debugPrint(" - 0x");
    debugPrint(ultoa(page + SPM_PAGESIZE, buff, 16));
    debugPrint("\n");
#endif
#if DEBUG >= 2
    debugPrint("Data: ");
    for (i = 0; i < SPM_PAGESIZE; i++) {
        uint8_t c = ((d[i] & 0xF0) >> 4);
        if (c > 9) {
            c = (c - 10) + 'A';
        } else {
            c += '0';
        }
        serialWrite(c);
        c = (d[i] & 0x0F);
        if (c > 9) {
            c = (c - 10) + 'A';
        } else {
            c += '0';
        }
        serialWrite(c);
        serialWrite(' ');
    }
    debugPrint("\n");
#endif

    cli();

    boot_page_erase_safe(page);
    for (i = 0; i < SPM_PAGESIZE; i += 2) {
        uint16_t w = *d++;
        w += ((*d++) << 8);
        boot_page_fill_safe(page + i, w);
    }
    boot_page_write_safe(page);
    boot_rww_enable_safe(); // Allows us to jump back

    sei(); // Interrupts are always on before calling this
}
