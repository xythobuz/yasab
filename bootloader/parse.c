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

#define DEBUG 0

#include "global.h"

#define isNum(x) ((x >= 0x30) && (x <= 0x39))
#define isLC(x) ((x >= 0x61) && (x <= 0x7A))
#define isUC(x) ((x >= 0x41) && (x <= 0x5A))
#define isValid(x) (isNum(x) || isLC(x) || isUC(x))

void parse(uint8_t c) {
    static uint8_t hexBuf[5];
    static uint8_t hexBufI = 0; // Buffer for reading hex nums
    static uint8_t buf[SPM_PAGESIZE]; // Data to flash
    static uint16_t flashI = 0;
    static uint16_t flashPage = 0; // flash page to be written

    // Hex File Record
    static uint8_t size = 0;
    static uint16_t address = 0;
    static uint8_t type = 0;
    static uint16_t checksum = 0;

    static uint8_t hexCount = 0; // Counter for chars in one row of hex file
    static uint8_t parseState = START;
    static uint8_t nextPage = 1; // Flag for new flash page

    if (parseState == START) {
        if (c == ':') {
            debugPrint("Start\n");
            parseState = SIZE;
            hexBufI = 0;
            checksum = 0;
        }
    } else if (parseState == SIZE) {
        if (isValid(c)) {
            hexBuf[hexBufI++] = c;
            if (hexBufI == 2) {
                debugPrint("Size\n");
                parseState = ADDRESS;
                hexBufI = 0;
                size = convert(hexBuf, 2);
                checksum += size;
            }
        }
    } else if (parseState == ADDRESS) {
        if (isValid(c)) {
            hexBuf[hexBufI++] = c;
            if (hexBufI == 4) {
                debugPrint("Address\n");
                parseState = TYPE;
                hexBufI = 0;
                address = convert(hexBuf, 4);
                checksum += (address & 0xFF);
                checksum += ((address & 0xFF00) >> 8);
                if (nextPage) {
                    flashPage = address - (address % SPM_PAGESIZE);
                    nextPage = 0;
                }
            }
        }
    } else if (parseState == TYPE) {
        if (isValid(c)) {
            hexBuf[hexBufI++] = c;
            if (hexBufI == 2) {
                debugPrint("Type\n");
                hexBufI = 0;
                hexCount = 0;
                type = convert(hexBuf, 2);
                checksum += type;
                if (type == 1) {
                    parseState = CHECKSUM;
                } else {
                    parseState = DATA;
                }
            }
        }
    } else if (parseState == DATA) {
        if (isValid(c)) {
            hexBuf[hexBufI++] = c;
            if (hexBufI == 2) {
                debugPrint("Data\n");
                hexBufI = 0;
                buf[flashI] = convert(hexBuf, 2);
                checksum += buf[flashI];
                flashI++;
                hexCount++;
                if (hexCount == size) {
                    parseState = CHECKSUM;
                    hexCount = 0;
                    hexBufI = 0;
                }
                if (flashI >= SPM_PAGESIZE) {
                    // Write page into flash
                    program(flashPage, buf);
                    set(buf, 0xFF, sizeof(buf));
                    flashI = 0;
                    nextPage = 1;
                }
            }
        }
    } else if (parseState == CHECKSUM) {
        if (isValid(c)) {
            hexBuf[hexBufI++] = c;
            if (hexBufI == 2) {
                debugPrint("Checksum\n");
                c = convert(hexBuf, 2);
                checksum += c;
                checksum &= 0x00FF;
                if (checksum == 0) {
                    parseState = START;
                    CHECKSUMVALID();
                } else {
                    parseState = ERROR;
                    CHECKSUMINVALID();
                }
                if (type == 1) {
                    // EOF, write rest
                    program(flashPage, buf);
                    appState = EXIT;
                    debugPrint("Finished\n");
                }
            }
        }
    } else {
        debugPrint("Error\n");
        PROGERROR();
        gotoApplication();
    }
}
