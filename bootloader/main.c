/*
 * main.c
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
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>
#include <avr/wdt.h>

#define DEBUG 0
#define VERSION "1.1"

#include "global.h"

uint8_t buf[SPM_PAGESIZE]; // Data to flash
volatile uint32_t systemTime = 0;

uint32_t readFourBytes(char c) {
    uint32_t i;
    i = (((uint32_t)c) << 24);
    i |= (((uint32_t)serialGetBlocking()) << 16);
    i |= (((uint32_t)serialGetBlocking()) << 8);
    i |= ((uint32_t)serialGetBlocking());
    return i;
}

void wdtDisable(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdtDisable(void) {
    MCUSR = 0;
    wdt_disable();
}

void main(void) __attribute__ ((noreturn));
void main(void) {
    static uint32_t dataPointer = 0;
    static uint32_t readPointer = 0;
    static uint32_t dataAddress;
    static uint32_t dataLength;
    static uint16_t bufPointer = 0;
    uint8_t c = 0;

    wdtDisable();

    // Move Interrupt Vectors into Bootloader Section
    c = INTERRUPTMOVE;
    INTERRUPTMOVE = c | (1 << IVCE);
    INTERRUPTMOVE = c | (1 << IVSEL);

    TCRA |= (1 << WGM21); // CTC Mode
#if F_CPU == 16000000
    TCRB |= (1 << CS22); // Prescaler: 64
    OCR = 250;
#else
#error F_CPU not compatible with timer module. DIY!
#endif
    TIMS |= (1 << OCIE); // Enable compare match interrupt

    serialInit(BAUD(BAUDRATE, F_CPU));
    sei();
    set(buf, 0xFF, sizeof(buf));

    debugPrint("YASAB ");
    debugPrint(VERSION);
    debugPrint(" by xythobuz\n");

    while (systemTime < BOOTDELAY) {
        if (serialHasChar()) {
            c = serialGet(); // Clear rx buffer
            goto ok; // Yeah, shame on me...
        }
    }
    gotoApplication();

ok:
    serialWrite(OKAY);
    while (!serialTxBufferEmpty()); // Wait till it's sent

    uint32_t t = systemTime;
    while (systemTime < (t + BOOTDELAY)) {
        if (serialHasChar()) {
            if (serialGet() == CONFIRM) {
                goto ack; // I deserve it...
            }
        }
    }
    gotoApplication();

ack:
    serialWrite(ACK);
    while (!serialTxBufferEmpty());

    dataAddress = readFourBytes(serialGetBlocking());
    serialWrite(OKAY);
    while (!serialTxBufferEmpty()); // Wait till it's sent
    debugPrint("Got address!\n");

    dataLength = readFourBytes(serialGetBlocking());
    if ((dataAddress + dataLength) >= BOOTSTART) {
        serialWrite(ERROR);
    } else {
        serialWrite(OKAY);
    }
    while (!serialTxBufferEmpty());
    debugPrint("Got length!\n");

    while (readPointer < dataLength) {
        buf[bufPointer] = serialGetBlocking();
        readPointer++;
        if (bufPointer < (SPM_PAGESIZE - 1)) {
            bufPointer++;
        } else {
            bufPointer = 0;
            program(dataPointer + dataAddress, buf);
            dataPointer += SPM_PAGESIZE;
            set(buf, 0xFF, sizeof(buf));
            serialWrite(OKAY);
        }
    }

    debugPrint("Got data!\n");

    if (bufPointer != 0) {
        program(dataPointer + dataAddress, buf);
        serialWrite(OKAY);
    }

    uint8_t sreg = SREG;
    cli();
    boot_spm_busy_wait();
    boot_rww_enable(); // Allows us to jump to application
    SREG = sreg;

    gotoApplication();
}

ISR(INTERRUPT) {
    systemTime++;
}
