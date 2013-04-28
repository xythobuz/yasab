/*
 * spm.c
 *
 * Copyright (c) 2012, Thomas Buck <xythobuz@me.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

void program(uint8_t uart, uint32_t page, uint8_t *d) {
    uint16_t i;
    uint8_t sreg = SREG;

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

    setFlow(uart, 0);
    cli();

    eeprom_busy_wait();
    boot_page_erase(page);
    boot_spm_busy_wait();
    for (i = 0; i < SPM_PAGESIZE; i += 2) {
        uint16_t w = *d++;
        w += (*d++) << 8;
        boot_page_fill(page + i, w);
    }
    boot_page_write(page);

    SREG = sreg;
    setFlow(uart, 1);
}
