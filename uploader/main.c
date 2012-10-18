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
    if (argc < 3) {
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

    // Open serial port
    if (serialOpen(argv[1]) != 0) {
        printf("Could not open port %s\n", argv[1]);
        return 1;
    }

    signal(SIGINT, intHandler);
    signal(SIGQUIT, intHandler);

    printf("Waiting for bootloader... Stop with CTRL+C\n");

    if (argc > 3) {
        writec(argv[3][0]); // TO-DO: Parse C Escape Sequences
    }

    if (!isValid()) {
        printf("HEX-File not valid!\n");
        suicide();
    }

    printf("Minimum Address: %X", minAddress());

    printf("Got response from YASAB! Sending HEX-File...\n");

    printf("YASAB confirmed upload!\n");
    freeHex();
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
int serialReadTry(char *data, size_t length) {
    int i = 0;
    int written = 0;
    int ret;
    time_t start, end;
    start = time(NULL);

    while (1) {
        ret = serialRead((data + written), (length - written));
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
            printf("Timeout while reading!\n");
            return 1;
        }
    }
    return 0;
}

// 1 on error, 0 on success
int serialWriteTry(char *data, size_t length) {
    int i = 0;
    int written = 0;
    int ret;
    while (1) {
        ret = serialWrite((data + written), (length - written));
        sync();
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
    }
    return 0;
}

char readc(void) {
    char c;
    if (serialReadTry(&c, 1) != 0) {
        printf("Error while receiving!\n");
        suicide();
    }
    return c;
}

void sync(void) {
    if (serialSync() != 0) {
        printf("Could not flush data!\n");
        suicide();
    }
}

void writec(char c) {
    if (serialWriteTry(&c, 1) != 0) {
        printf("Error while sending!\n");
        suicide();
    }
    sync();
}

void intHandler(int dummy) {
    serialClose();
    freeHex();
    exit(1);
}
