/*
 * global.h
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
#include "serial.h"

#ifndef BOOTDELAY
#define BOOTDELAY 2000
#endif

#ifndef BAUDRATE
#define BAUDRATE 38400
#endif

#ifdef __AVR_ATmega32__
#define INTERRUPTMOVE GICR
#define INTERRUPT TIMER2_COMP_vect
#elif defined(__AVR_ATmega2560__)
#define INTERRUPTMOVE MCUCR
#define INTERRUPT TIMER2_COMPA_vect
#else
#error MCU not compatible with timer module. DIY!
#endif

#define ERROR 'e'
#define OKAY 'o'
#define FLASH 'f'
#define CONFIRM 'c'
#define ACK 'a'

// On an AVR with more than one UART, you can select which one to use (0 or 1) by setting a switch before boot:
#define SELECTDDR DDRJ
#define SELECTPORT PORTJ
#define SELECTPIN PINJ
#define SELECTBIT PJ1
#define PLUSBIT PJ0

// Debug Output
#if defined(DEBUG) && (DEBUG > 0)
#define debugPrint(x) serialWriteString(0, x)
#else
#define debugPrint(ignore)
#endif

extern uint8_t buf[SPM_PAGESIZE];

void parse(void);
void set(uint8_t *d, uint8_t c, uint16_t l);
void program(uint8_t uart, uint32_t page, uint8_t *d);
void gotoApplication(void);

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega2560__)
#define TCRA TCCR2A
#define TCRB TCCR2B
#define OCR OCR2A
#define TIMS TIMSK2
#define OCIE OCIE2A
#elif defined(__AVR_ATmega32__)
#define TCRA TCCR2
#define TCRB TCCR2
#define OCR OCR2
#define TIMS TIMSK
#define OCIE OCIE2
#else
#error MCU not compatible with timer module. DIY!
#endif
