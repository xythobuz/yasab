/*
 * parser.c
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
        fprintf(stderr, "%i: Checksum invalid (%X != %X)!\n", line, checksum, 0);
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
        fprintf(stderr, "Not enough memory (%lu bytes)\n", lines * sizeof(char *));
        return 1;
    }

    // Copy hex file
    fseek(fp, 0L, SEEK_SET);
    while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
        // Allocate memory
        hexFile[hexFileLines] = (char *)malloc((strlen(linebuf) + 1) * sizeof(char));
        if (hexFile[hexFileLines] == NULL) {
            fprintf(stderr, "Not enough memory (%lu bytes)\n", (strlen(linebuf) + 1) * sizeof(char));
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
