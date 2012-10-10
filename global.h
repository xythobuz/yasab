/*
 * global.h
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
#include "serial.h"

#define BAUDRATE 38400
#define BOOTDELAY 1000

#define XON() serialWrite(17)
#define XOFF() serialWrite(19)

#define PROGRAMMED() serialWrite('P');
#define PROGERROR() serialWrite('E');

// appState: Bootloader State
#define WAITING 0
#define PARSING 1
#define EXIT 2

// Parser State
#define START 0
#define SIZE 1
#define ADDRESS 2
#define TYPE 3
#define DATA 4
#define CHECKSUM 5
#define ERROR 6

extern uint8_t appState;

void parse(uint8_t c);
void set(uint8_t *d, uint8_t c, uint16_t l);
uint16_t convert(uint8_t *d, uint8_t l);
void program(uint32_t page, uint8_t *d);
void gotoApplication(void);
