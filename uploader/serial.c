/*
 * serial.c
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
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <poll.h>

#include "serial.h"

#define XONXOFF
#define SEARCH "tty"
#define TIMEOUT 2 // in seconds
#define XON 0x11
#define XOFF 0x13

int serialOpen(char *port, int baud, int flowcontrol) {
    int fd;
    struct termios options;

    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        return -1;
    }

    tcgetattr(fd, &options);

    options.c_lflag = 0;
    options.c_oflag = 0;
    options.c_iflag = 0;

    // Set Baudrate
    switch (baud) {
        case 9600:
            cfsetispeed(&options, B9600);
            cfsetospeed(&options, B9600);
            break;
        case 19200:
            cfsetispeed(&options, B19200);
            cfsetospeed(&options, B19200);
            break;
        case 38400:
            cfsetispeed(&options, B38400);
            cfsetospeed(&options, B38400);
            break;
        case 76800:
            cfsetispeed(&options, B76800);
            cfsetospeed(&options, B76800);
            break;
        case 115200:
            cfsetispeed(&options, B115200);
            cfsetospeed(&options, B115200);
            break;
        default:
            fprintf(stderr, "Warning: Baudrate not supported!\n");
            serialClose(fd);
            return -1;
    }

    // Input Modes
    options.c_iflag |= IGNCR; // Ignore CR
#ifdef XONXOFF
    options.c_iflag |= IXON; // XON-XOFF Flow Control
#endif

    // Output Modes
    options.c_oflag |= OPOST; // Post-process output

    // Control Modes
    options.c_cflag |= CS8; // 8 data bits
    options.c_cflag |= CREAD; // Enable Receiver
    options.c_cflag |= CLOCAL; // Ignore modem status lines

    // Local Modes
    options.c_lflag |= IEXTEN; // Extended input character processing

    // Special characters
    options.c_cc[VMIN] = 0; // Always return...
    options.c_cc[VTIME] = 0; // ..immediately from read()
    options.c_cc[VSTOP] = XOFF;
    options.c_cc[VSTART] = XON;

    tcsetattr(fd, TCSANOW, &options);

    tcflush(fd, TCIOFLUSH);

    return fd;
}

int serialHasChar(int fd) {
    struct pollfd fds;
    fds.fd = fd;
    fds.events = (POLLIN | POLLPRI); // Data may be read
    if (poll(&fds, 1, 0) > 0) {
        return 1;
    } else {
        return 0;
    }
}

typedef ssize_t(*Func)(int, void *, size_t);

int serialRaw(int fd, char *d, int len, Func f) {
    int errors = 0;
    const int maxError = 1;
    int processed = 0;
    time_t start = time(NULL), end;

    while (processed < len) {
        int t = f(fd, (d + processed), (len - processed));
        if (t == -1) {
            errors++;
            if (errors >= maxError) {
                fprintf(stderr, "Error while reading/writing: %s\n", strerror(errno));
                return 0;
            }
        } else {
            processed += t;
        }

        end = time(NULL);
        if (difftime(end, start) > TIMEOUT) {
            fprintf(stderr, "Timeout (%is) while reading/writing!\n", TIMEOUT);
            return 0;
        }
    }
    return len;
}

void serialWaitUntilSent(int fd) {
    while (tcdrain(fd) == -1) {
        fprintf(stderr, "Could not drain data: %s\n", strerror(errno));
    }
}

int serialWriteRaw(int fd, char *d, int len) {
    int i = serialRaw(fd, d, len, (Func)write);
    serialWaitUntilSent(fd);
    return i;
}

int serialReadRaw(int fd, char *d, int len) {
    return serialRaw(fd, d, len, (Func)read);
}

void serialWriteChar(int fd, char c) {
    while (serialWriteRaw(fd, &c, 1) != 1);
}

void serialReadChar(int fd, char *c) {
    while (serialReadRaw(fd, c, 1) != 1);
    if (*c == XON) {
        if (tcflow(fd, TCOON) == -1) {
            fprintf(stderr, "Could not restart flow: %s\n", strerror(errno));
        }
        serialReadChar(fd, c);
    } else if (*c == XOFF) {
        if (tcflow(fd, TCOOFF) == -1) {
            fprintf(stderr, "Could not stop flow: %s\n", strerror(errno));
        }
        serialReadChar(fd, c);
    }
}

void serialWriteString(int fd, char *s) {
    int l = strlen(s);
    for (int i = 0; i < l; i++) {
        serialWriteChar(fd, s[i]);
    }
}

void serialClose(int fd) {
    tcflush(fd, TCIOFLUSH);
    close(fd);
}

char** namesInDev(int *siz) {
    DIR *dir;
    struct dirent *ent;
    int i = 0, size = 0;
    char **files = NULL;
    dir = opendir("/dev/");
    while ((ent = readdir(dir)) != NULL) {
        size++;
    }
    files = (char **)malloc((size + 1) * sizeof(char *));
    files[size++] = NULL;
    closedir(dir);
    dir = opendir("/dev/");
    while ((ent = readdir(dir)) != NULL) {
        files[i] = (char *)malloc((strlen(ent->d_name) + 1) * sizeof(char));
        files[i] = strcpy(files[i], ent->d_name);
        i++;
    }
    closedir(dir);

    char *tmp = NULL;
    // Fix every string, add /dev/ in front of it...
    for (i = 0; i < (size - 1); i++) {
        tmp = (char *)malloc((strlen(files[i]) + 6) * sizeof(char));
        tmp[0] = '/';
        tmp[1] = 'd';
        tmp[2] = 'e';
        tmp[3] = 'v';
        tmp[4] = '/';
        files[i] = strncat(tmp, files[i], strlen(files[i]));
    }

    *siz = size;
    return files;
}

char** getSerialPorts(void) {
    int size;
    char** files = namesInDev(&size);
    char **fin = NULL, **finish = NULL;
    int i = 0, j = 0, f, g;

    fin = (char **)malloc(size * sizeof(char *));
    // Has space for all files in dev!

    while (files[i] != NULL) {
        // Filter for SEARCH and if it is a serial port
        if (strstr(files[i], SEARCH) != NULL) {
            fin[j++] = files[i];
        } else {
            free(files[i]);
        }
        i++;
    }
    free(files);

    // Copy in memory with matching size, NULL at end
    finish = (char **)malloc((j + 1) * sizeof(char *));
    finish[j] = NULL;
    for (i = 0; i < j; i++) {
        finish[i] = fin[i];
    }

    free(fin);
    return finish;
}
