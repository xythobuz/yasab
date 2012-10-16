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
#include <util/delay.h>

#define DEBUG 0

#include "global.h"

uint8_t buf[SPM_PAGESIZE]; // Data to flash

/*
 * 0: Getting address
 * 1: Getting length
 * 2: Reading data
 */
uint8_t parseState = 0;
uint32_t dataPointer = 0;
uint32_t dataAddress = 0;
uint32_t dataLength = 0;
uint16_t bufPointer = 0;

#define page(x) (x - (x % SPM_PAGESIZE))

void parse(void) {
    uint8_t c;

    if (!serialHasChar()) {
        return;
    }
    c = serialGet();

    if (parseState == 0) {
        dataAddress = (((uint32_t)c) << 24);
        while(!serialHasChar());
        dataAddress |= (((uint32_t)serialGet()) << 16);
        while(!serialHasChar());
        dataAddress |= (((uint32_t)serialGet()) << 8);
        while(!serialHasChar());
        dataAddress |= ((uint32_t)serialGet());
        serialWrite(OKAY);
        while (!serialTxBufferEmpty()); // Wait till it's sent
        debugPrint("Got address!\n");
        parseState++;
    } else if (parseState == 1) {
        dataLength = (((uint32_t)c) << 24);
        while(!serialHasChar());
        dataLength |= (((uint32_t)serialGet()) << 16);
        while(!serialHasChar());
        dataLength |= (((uint32_t)serialGet()) << 8);
        while(!serialHasChar());
        dataLength |= ((uint32_t)serialGet());
        serialWrite(OKAY);
        while (!serialTxBufferEmpty()); // Wait till it's sent
        debugPrint("Got length!\n");
        parseState++;
    } else {
        dataPointer = dataAddress;
        while (dataPointer < (dataLength + dataAddress)) {
            while (!serialHasChar());
            buf[bufPointer] = serialGet();
            dataPointer++;
            if (bufPointer < (SPM_PAGESIZE - 1)) {
                bufPointer++;
            } else {
                bufPointer = 0;
                program(page(dataPointer), buf);
                set(buf, 0xFF, sizeof(buf));
                serialWrite(OKAY);
                while (!serialTxBufferEmpty());
            }
        }
        debugPrint("Got data!\n");
        if (bufPointer != 0) {
            program(page(dataPointer), buf);
            serialWrite(OKAY);
            while (!serialTxBufferEmpty());
        }
        gotoApplication();
    }
}
