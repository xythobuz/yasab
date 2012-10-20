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

#define DEBUG 0
#define VERSION "1.0"

#include "global.h"

uint8_t buf[SPM_PAGESIZE]; // Data to flash

uint32_t readFourBytes(char c) {
    uint32_t i;
    i = (((uint32_t)c) << 24);
    i |= (((uint32_t)serialGetBlocking()) << 16);
    i |= (((uint32_t)serialGetBlocking()) << 8);
    i |= ((uint32_t)serialGetBlocking());

    serialWrite(OKAY);
    while (!serialTxBufferEmpty()); // Wait till it's sent
    return i;
}

void main(void) __attribute__ ((noreturn));
void main(void) {
    static uint32_t dataPointer = 0;
    static uint32_t readPointer = 0;
    static uint32_t dataAddress;
    static uint32_t dataLength;
    static uint16_t bufPointer = 0;
    uint8_t c = 0;

    // Move Interrupt Vectors into Bootloader Section
    c = GICR;
    GICR = c | (1 << IVCE);
    GICR = c | (1 << IVSEL);

    serialInit(BAUD(BAUDRATE, F_CPU));
    sei();
    set(buf, 0xFF, sizeof(buf));

    _delay_ms(BOOTDELAY);

    debugPrint("YASAB ");
    debugPrint(VERSION);
    debugPrint(" by xythobuz\n");

    while (serialHasChar()) {
        c = serialGet(); // Clear rx buffer
    }

    serialWrite(OKAY);
    while (!serialTxBufferEmpty()); // Wait till it's sent

    if (serialGetBlocking() != CONFIRM) {
        gotoApplication();
    }

    serialWrite(ACK);
    while (!serialTxBufferEmpty());

    dataAddress = readFourBytes(serialGetBlocking());
    debugPrint("Got address!\n");

    dataLength = readFourBytes(serialGetBlocking());
    debugPrint("Got length!\n");

    while (readPointer < dataLength) {
        buf[bufPointer] = serialGetBlocking();
        readPointer++;
        if (bufPointer < (SPM_PAGESIZE - 1)) {
            bufPointer++;
        } else {
            bufPointer = 0;
            setFlow(0);
            program(dataPointer + dataAddress, buf);
            setFlow(1);
            dataPointer += SPM_PAGESIZE;
            set(buf, 0xFF, sizeof(buf));
            serialWrite(OKAY);
        }
    }

    debugPrint("Got data!\n");

    if (bufPointer != 0) {
        setFlow(0);
        program(dataPointer + dataAddress, buf);
        setFlow(1);
        serialWrite(OKAY);
    }

    gotoApplication();
}
