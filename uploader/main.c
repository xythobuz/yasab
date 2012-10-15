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
#include <strings.h>
#include <time.h>

#include <unistd.h> // usleep()

#include "serial.h"

#define BAUDRATE 38400 // Change baudrate in serial.c and winSerial.c too
#define DELAY ((1000000/(BAUDRATE/8)) * 2)

void printProgress(long a, long b);
void printNextPage(void);
char readc(void);
void writec(char c);
int serialWriteString(char *s);
int serialReadTry(char *data, size_t length);
int serialWriteTry(char *data, size_t length);
void intHandler(int dummy);

FILE *fp = NULL;
char hello[] = { 'H', 'E', 'X', '?', '\n' };
char okay[] = { 'O', 'K', '!', '\n' };
#define ERROR 'e'
#define OK 'p'
#define XON 0x11
#define XOFF 0x13

#define suicide() intHandler(0)

int main(int argc, char *argv[]) {
    char c, f, r = '?';
    int i;

    if (argc < 3) {
#ifdef _WIN32
        printf("Usage:\n%s COM1 sample.hex [q]\n", argv[0]);
#else
        printf("Usage:\n%s /dev/port /path/to.hex [q]\n", argv[0]);
#endif
        return 1;
    }

    if (serialOpen(argv[1]) != 0) {
        printf("Could not open port %s\n", argv[1]);
        return 1;
    }

    signal(SIGINT, intHandler);
    signal(SIGQUIT, intHandler);

    if ((fp = fopen(argv[2], "r")) == NULL) {
        printf("Could not open file %s\n", argv[2]);
        suicide();
    }

    printf("Waiting for bootloader...\nStop with CTRL+C\n");

    if (argc > 3) {
        r = argv[3][0];
    }

    writec(r);
    while (1) {
        if (serialRead(&c, 1) == 1) {
            break; // Got response
        }
        usleep(DELAY);
        writec(r);
    }
    i = 0;
    while (1) {
        if (c == hello[i]) {
            putc(c, stdout);
            i++;
            if (i < sizeof(hello))
                c = readc();
            else
                break;
        } else {
            printf("Answer not from YASAB?! (%x - %c)\n", c, c);
            // suicide();
            c = readc();
        }
    }

    printf("Got response from YASAB! Sending HEX-File...\n");

    fseek(fp, 0L, SEEK_END);
    long max = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    long d = 0;
    while (1) {
        if (serialRead(&c, 1) == 1) {
            if (c == XOFF) {
            } else if (c == XON) {
            } else if (c == OK) {
                printNextPage();
            } else if (c == ERROR) {
                printf("\n\nAn Error in YASAB occurred!\n");
                suicide();
            } else if (c == okay[0]) {
                break;
            } else {
                printf("\nReceived %x - %c\n", c, c);
            }
        }
        if ((f = getc(fp)) == EOF) {
            continue;
        }
        writec(f);
        printProgress(++d, max);
        usleep(DELAY);
    }

    printf("\nHEX File sent!\n");

    i = 0;
    while (1) {
        if (c == okay[i]) {
            putc(c, stdout);
            i++;
            if (i < sizeof(okay))
                c = readc();
            else
                break;
        } else {
            printf("No valid success response?! (%x - %c)\n", c, c);
            // suicide();
            c = readc();
        }
    }

    printf("YASAB confirmed upload!\n");

    fclose(fp);
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

int serialWriteString(char *s) {
    return serialWriteTry(s, strlen(s));
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

void intHandler(int dummy) {
    if (fp != NULL) {
        fclose(fp);
    }
    serialClose();
    exit(1);
}
