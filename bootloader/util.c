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

    TCRA = TCRB = TIMS = 0;

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
