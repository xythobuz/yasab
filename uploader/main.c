/*
 * main.c
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
#include <stdlib.h>
#include <time.h>

#include "serial.h"
#include "parser.h"

void printProgress(long a, long b);
void printNextPage(void);
void intHandler(int dummy);
#define suicide() intHandler(0)

#define OKAY 'o'
#define ERROR 'e'
#define CONFIRM 'c'
#define ACK 'a'

int fd = -1;

int main(int argc, char *argv[]) {
    FILE *fp;
    char c;
    time_t start = time(NULL), end;
    if ((argc < 3) || (argc > 4)) {
        printf("Usage:\n%s /dev/port /path/to.hex [q]\n", argv[0]);
        return 1;
    }

    // Open HEX File
    if ((fp = fopen(argv[2], "r")) == NULL) {
        printf("Could not open file %s\n", argv[2]);
        return 1;
    }

    // Read it's contents
    if (readHex(fp) != 0) {
        printf("Could not parse HEX file %s\n", argv[2]);
        fclose(fp);
        return 1;
    }

    fclose(fp);

    if (!isValid()) {
        printf("HEX-File not valid!\n");
        freeHex();
        return 1;
    }

    uint32_t min = minAddress();
    uint32_t max = maxAddress();
    uint32_t length = dataLength();
    printf("Hex File Path   : %s\n", argv[2]);
    printf("Minimum Address : 0x%X\n", min);
    printf("Maximum Address : 0x%X\n", max);
    printf("Data payload    : %i bytes\n\n", length);

    uint8_t *d = (uint8_t *)malloc(length * sizeof(uint8_t));
    if (d == NULL) {
        fprintf(stderr, "Not enough memory (%lu bytes)!\n", length * sizeof(uint8_t));
        freeHex();
        return 1;
    }
    parseData(d);
    freeHex();

    // Open serial port
    if ((fd = serialOpen(argv[1], 38400, 1)) == -1) {
        printf("Could not open port %s\n", argv[1]);
        free(d);
        return 1;
    }

    signal(SIGINT, intHandler);
    signal(SIGQUIT, intHandler);

    printf("Pinging bootloader... Stop with CTRL+C\n");

ping:
    if (argc > 3) {
        c = argv[3][0];
    } else {
        c = 'f';
    }
    serialWriteChar(fd, c);
    serialReadChar(fd, &c);
    if (c != OKAY) {
        printf("Received strange byte (%x). WTF?!\n", c, c);
        goto ping;
    }

    printf("Got response... Acknowledging...\n");
    serialWriteChar(fd, CONFIRM);
    serialReadChar(fd, &c);
    if (c != ACK) {
        printf("Invalid acknowledge from YASAB (%x)! Trying again...\n", c, c);
        goto ping;
    }

    printf("Connection established successfully!\n");
    printf("Sending target address...\n");

    serialWriteChar(fd, (min & 0xFF000000) >> 24);
    serialWriteChar(fd, (min & 0x00FF0000) >> 16);
    serialWriteChar(fd, (min & 0x0000FF00) >> 8);
    serialWriteChar(fd, min & 0x000000FF);

    serialReadChar(fd, &c);
    if (c != OKAY) {
        printf("Invalid acknowledge from YASAB (%x)!\n", c, c);
        free(d);
        serialClose(fd);
        return 1;
    }

    printf("Sending data length...\n");

    serialWriteChar(fd, (length & 0xFF000000) >> 24);
    serialWriteChar(fd, (length & 0x00FF0000) >> 16);
    serialWriteChar(fd, (length & 0x0000FF00) >> 8);
    serialWriteChar(fd, length & 0x000000FF);

    serialReadChar(fd, &c);
    if (c != OKAY) {
        printf("Invalid acknowledge from YASAB (%x)!\n", c, c);
        free(d);
        serialClose(fd);
        return 1;
    }

    for (uint32_t i = 0; i < length; i++) {
        serialWriteChar(fd, d[i]);
        if (serialHasChar(fd)) {
            serialReadChar(fd, &c);
            if (c == OKAY) {
                printNextPage();
            } else if (c == ERROR) {
                printf("YASAB aborted the connection!\n");
                free(d);
                serialClose(fd);
                return 1;
            } else {
                printf("Unknown answer from YASAB (%x)!\n", c, c);
            }
        }
        printProgress(i + 1, length);
    }

    end = time(NULL);
    printf("\n\nUpload finished after %3.1f seconds.\n", difftime(end, start));
    printf("YASAB - Yet another simple AVR Bootloader\n");
    printf("By xythobuz - Visit www.xythobuz.org\n");

    free(d);
    serialClose(fd);
    return 0;
}

void printNextPage(void) {
    static int i = 1;
    printf(" %i page(s) written!", i++);
}

void printProgress(long a, long b) {
    long c = (a * 100) / b;
    printf("\r%li%% (%li / %li)", c, a, b);
    fflush(stdout);
}

void intHandler(int dummy) {
    if (fd != -1)
        serialClose(fd);
    exit(1);
}
