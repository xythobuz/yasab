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
#include <unistd.h> // usleep()

#include "serial.h"
#include "parser.h"

char readc(void);
void writec(char c);
void sync(void);
void printProgress(long a, long b);
void printNextPage(void);
void intHandler(int dummy);
#define suicide() intHandler(0)

#define OKAY 'o'
#define ERROR 'e'
#define FLASH 'f'
#define CONFIRM 'c'

int main(int argc, char *argv[]) {
    FILE *fp;
    char c;
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
    printf("Data payload    : %i bytes\n", length);

    uint8_t *d = (uint8_t *)malloc(length * sizeof(uint8_t));
    if (d == NULL) {
        printf("Not enough memory!\n");
        freeHex();
        return 1;
    }
    parseData(d);
    freeHex();

    // Open serial port
    if (serialOpen(argv[1]) != 0) {
        printf("Could not open port %s\n", argv[1]);
        free(d);
        return 1;
    }

    signal(SIGINT, intHandler);
    signal(SIGQUIT, intHandler);

    if (argc > 3) {
        writec(argv[3][0]); // TO-DO: Parse C Escape Sequences
    }

    printf("Pinging bootloader... Stop with CTRL+C\n");

    int i = 0;
    while(1) {
        writec(FLASH);
        i++;
        if (serialRead(&c, 1) == 1) {
            if (c == OKAY) {
                break;
            } else {
                printf("Received strange byte (%c - %x). WTF?!\n", c, c);
            }
        }
        usleep(500000);
    }

    printf("Got response after %i attempts. Acknowledging...\n", i);
    writec(CONFIRM);
    writec(CONFIRM);
    writec(CONFIRM);

    c = readc();
    if (c != OKAY) {
        printf("Invalid acknowledge from YASAB (%c - %x)!\n", c, c);
        free(d);
        serialClose();
        return 1;
    }

    printf("Connection established successfully!\n");
    printf("Sending target address...\n");

    writec((min & 0xFF000000) >> 24);
    writec((min & 0x00FF0000) >> 16);
    writec((min & 0x0000FF00) >> 8);
    writec(min & 0x000000FF);

    c = readc();
    if (c != OKAY) {
        printf("Invalid acknowledge from YASAB (%c - %x)!\n", c, c);
        free(d);
        serialClose();
        return 1;
    }

    printf("Sending data length...\n");

    writec((length & 0xFF000000) >> 24);
    writec((length & 0x00FF0000) >> 16);
    writec((length & 0x0000FF00) >> 8);
    writec(length & 0x000000FF);

    c = readc();
    if (c != OKAY) {
        printf("Invalid acknowledge from YASAB (%c - %x)!\n", c, c);
        free(d);
        serialClose();
        return 1;
    }

    for (uint32_t i = 0; i < length; i++) {
        writec(d[i]);
        if (serialRead(&c, 1) == 1) {
            if (c == OKAY) {
                printNextPage();
            } else if (c == ERROR) {
                printf("YASAB aborted the connection!\n");
                free(d);
                serialClose();
                return 1;
            } else {
                printf("Unknown answer from YASAB (%c - %x)!\n", c, c);
            }
        }
        printProgress(i + 1, length);
    }

    printf("Upload finished. Thank you!\n");
    printf("YASAB - Yet another simple AVR Bootloader\n");
    printf("By xythobuz - Visit www.xythobuz.org\n");

    free(d);
    serialClose();
    return 0;
}

void printNextPage(void) {
    static int i = 0;
    for (int j = 0; j < i; j++) {
        printf(" ");
    }
    i++;
    printf(" Next page written!");
}

void printProgress(long a, long b) {
    long c = (a * 100) / b;
    printf("\r%li%% (%li / %li)", c, a, b);
    fflush(stdout);
}

// 1 on error, 0 on success
int serialTry(char *data, size_t length, ssize_t (*ser)(char *, size_t)) {
    int i = 0;
    int written = 0;
    int ret;
    time_t start, end;
    start = time(NULL);
    while (1) {
        ret = ser((data + written), (length - written));
        if (ret == -1) {
            i++;
        } else {
            written += ret;
        }
        if (i > 10) {
            return 1;
        }
        if (written == length) {
            break;
        }
        end = time(NULL);
        if (difftime(start, end) > 2) {
            printf("Timeout!\n");
            return 1;
        }
    }
    return 0;
}

void sync(void) {
    if (serialSync() != 0) {
        printf("Could not sync data!\n");
        suicide();
    }
}

char readc(void) {
    char c;
    if (serialTry(&c, 1, serialRead) != 0) {
        printf("Error while reading!\n");
        suicide();
    }
    return c;
}

void writec(char c) {
    if (serialTry(&c, 1, serialWrite) != 0) {
        printf("Error while sending!\n");
        suicide();
    }
    sync();
}

void intHandler(int dummy) {
    serialClose();
    exit(1);
}
