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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <unistd.h> // usleep

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

#define PINGDELAY 10 // in milliseconds

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

ping:
    printf("Pinging bootloader... Stop with CTRL+C\n");

    if (argc > 3) {
        c = argv[3][0];
    } else {
        c = 'f';
    }
    serialWriteChar(fd, c);
    usleep(PINGDELAY * 1000);
    if ((serialReadRaw(fd, &c, 1) != 1) || (c != OKAY)) {
        goto ping;
    }

    printf("Got response... Acknowledging...\n");
    serialWriteChar(fd, CONFIRM);
    serialReadChar(fd, &c);
    if (c != ACK) {
        printf("Invalid acknowledge! Trying again...\n", c, c);
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
        printf("Invalid acknowledge from YASAB!\n", c, c);
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

    printf("\n\n");

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

int pagesWritten = 0;

void printNextPage(void) {
    pagesWritten++;
}

void printProgress(long a, long b) {
    char cs[] = {'|', '|', '/', '/', '-', '-', '\\', '\\'};
    int csl = 8;
    static int i = 0;
    long c = (a * 100) / b;

    printf("\r\033[1A"); // Go to beginning of current line, then one line up

    printf("%li/%li bytes. %i page(s) programmed! %c\n", a, b, pagesWritten, cs[i]);
    if (i < (csl - 1)) {
        i++;
    } else {
        i = 0;
    }
    printf("Sending |");
    for (int i = 0; i < (c / 2); i++) {
        printf("#");
    }
    for (int i = (c / 2); i < 50; i++) {
        printf("-");
    }
    printf("|  %li%%", c);

    fflush(stdout);
}

void intHandler(int dummy) {
    if (fd != -1)
        serialClose(fd);
    exit(2);
}
