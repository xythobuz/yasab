/*
 * parser.h
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of YASAB.
 *
 * YASAB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YASAB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YASAB.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <stdint.h>

typedef struct {
    int valid;
    uint8_t length;
    uint16_t address;
    int type;
    uint8_t *data;
} HexLine;

uint16_t parseDigit(char *d, int len);
HexLine *parseLine(int line);

int readHex(FILE *fp);
void freeHex(void);

uint16_t minAddress(void);
int isValid(void);
