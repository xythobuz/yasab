/*
 * parser.c
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
#include <stdlib.h>
#include <string.h>

#include "parser.h"

#define debugPrint printf

char **hexFile = NULL;
int hexFileLines = 0;

void parseData(uint8_t *d) {
    uint32_t offset = 0;
    uint32_t start = minAddress();
    for (int i = 0; i < hexFileLines; i++) {
        HexLine *h = parseLine(i);
        uint32_t a = h->address + (offset << 4);

        if (h->type == 0x00) {
            for (int j = 0; j < h->length; j++) {
                d[(a - start) + j] = h->data[j];
            }
        } else if (h->type == 0x02) {
            offset = parseDigit(h->data, 4);
        }

        free(h->data);
        free(h);
    }
}

uint32_t dataLength(void) {
    uint32_t l = 0;
    for (int i = 0; i < hexFileLines; i++) {
        HexLine *h = parseLine(i);
        if (h->type == 0x00) {
            l += h->length;
        }
        free(h->data);
        free(h);
    }
    return l;
}

uint16_t parseDigit(char *d, int len) {
    uint16_t val = 0;
    for (int i = 0; i < len; i++) {
        char c = d[i];
        if ((c >= '0') && (c <= '9')) {
            c -= '0';
        } else if ((c >= 'A') && (c <= 'F')) {
            c -= 'A' - 10;
        } else if ((c >= 'a') && (c <= 'f')) {
            c -= 'a' - 10;
        }
        val = (16 * val) + c;
    }
    return val;
}

HexLine *parseLine(int line) {
    HexLine *h = (HexLine *)malloc(sizeof(HexLine));
    if (h == NULL) {
        return NULL;
    }

    // Record Mark
    if (hexFile[line][0] != ':') {
        h->valid = 0;
        return h;
    }

    h->length = parseDigit(hexFile[line] + 1, 2); // Record length
    h->address = parseDigit(hexFile[line] + 3, 4); // Load Offset
    h->type = parseDigit(hexFile[line] + 7, 2); // Record Type
    uint16_t checksum = h->length + (h->address & 0x00FF) + h->type;
    checksum += ((h->address & 0xFF00) >> 8);

    // Memory for data
    h->data = (uint8_t *)malloc(h->length * sizeof(uint8_t));
    if (h->data == NULL) {
        free(h);
        return NULL;
    }

    for (int i = 0; i < h->length; i++) {
        h->data[i] = parseDigit(hexFile[line] + 9 + (2 * i), 2);
        checksum += h->data[i];
    }

    checksum += parseDigit(hexFile[line] + 9 + (2 * h->length), 2);

    if ((checksum & 0x00FF) != 0) {
        h->valid = 0;
        debugPrint("%i: Checksum invalid (%X != %X)!\n", line, checksum, 0);
    } else {
        h->valid = 1;
    }

    return h;
}

uint32_t maxAddress(void) {
    uint32_t maxAdd = 0;
    uint32_t offset = 0;
    for (int i = 0; i < hexFileLines; i++) {
        HexLine *h = parseLine(i);
        uint32_t a = h->address + (offset << 4);
        if (h->type == 0x00) {
            if (a > maxAdd) {
                maxAdd = a;
            }
        } else if (h->type == 0x02) {
            offset = parseDigit(h->data, 4);
        }
        free(h->data);
        free(h);
    }
    return maxAdd;
}

uint32_t minAddress(void) {
    uint32_t minAdd = UINT16_MAX;
    uint32_t offset = 0;
    for (int i = 0; i < hexFileLines; i++) {
        HexLine *h = parseLine(i);
        uint32_t a = h->address + (offset << 4);
        if (h->type == 0x00) {
            if (a < minAdd) {
                minAdd = a;
            }
        } else if (h->type == 0x02) {
            offset = parseDigit(h->data, 4);
        }
        free(h->data);
        free(h);
    }
    return minAdd;
}

int isValid(void) {
    for (int i = 0; i < hexFileLines; i++) {
        HexLine *h = parseLine(i);
        if (h->valid == 0) {
            return 0;
        }
        if (h->type >= 0x04) {
            return 0;
        }
        free(h->data);
        free(h);
    }
    return 1;
}

int readHex(FILE *fp) {
    char linebuf[80]; // Should be enough for hex files

    // Count lines
    int lines = 0;
    fseek(fp, 0L, SEEK_SET); // Go to start
    while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
        lines++;
    }

    // Allocate memory for hex file lines
    hexFile = (char **)malloc(lines * sizeof(char *));
    if (hexFile == NULL) {
        return 1;
    }

    // Copy hex file
    fseek(fp, 0L, SEEK_SET);
    while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
        // Allocate memory
        hexFile[hexFileLines] = (char *)malloc((strlen(linebuf) + 1) * sizeof(char));
        if (hexFile[hexFileLines] == NULL) {
            freeHex();
            return 1;
        }

        // Copy it
        strcpy(hexFile[hexFileLines], linebuf);
        hexFileLines++;
    }

    return 0;
}

void freeHex(void) {
    int i;
    if (hexFile != NULL) {
        for (i = 0; i < hexFileLines; i++) {
            if (hexFile[i] != NULL) {
                free(hexFile[i]);
            }
        }
        free(hexFile);
    }
}
