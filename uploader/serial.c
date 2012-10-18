/*
 * POSIX compatible serial port library
 * Uses 8 databits, no parity, 1 stop bit, no handshaking
 *
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

#include "serial.h"

int fd = -1;

// Open the serial port
int serialOpen(char *port) {
    struct termios options;

    if (fd != -1) {
        close(fd);
    }
    fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        return -1;
    }

    fcntl(fd, F_SETFL, FNDELAY); // read() isn't blocking'
    tcgetattr(fd, &options);
    cfsetispeed(&options, BAUD); // Set speed
    cfsetospeed(&options, BAUD);

    // Local flags
    options.c_lflag = 0; // No local flags
    options.c_lflag &= ~ICANON; // Don't canonicalise
    options.c_lflag &= ~ECHO; // Don't echo
    options.c_lflag &= ~ECHOK; // Don't echo

    // Control flags
    options.c_cflag &= ~CRTSCTS; // Disable RTS/CTS
    options.c_cflag |= CLOCAL; // Ignore status lines
    options.c_cflag |= CREAD; // Enable receiver
    options.c_cflag |= HUPCL; // Drop DTR on close
    options.c_cflag &= ~PARENB; // 8N1
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    // oflag - output processing
    options.c_oflag &= ~OPOST; // No output processing
    options.c_oflag &= ~ONLCR; // Don't convert linefeeds

    // iflag - input processing
    options.c_iflag |= IGNPAR; // Ignore parity
    options.c_iflag &= ~ISTRIP; // Don't strip high order bit
    options.c_iflag |= IGNBRK; // Ignore break conditions
    options.c_iflag &= ~INLCR; // Don't Map NL to CR
    options.c_iflag &= ~ICRNL; // Don't Map CR to NL

#ifdef XONXOFF
    options.c_iflag |= (IXON | IXOFF | IXANY); // XON-XOFF Flow Control
#else
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
#endif

    tcsetattr(fd, TCSANOW, &options);

    return 0;
}

// Write to port. Returns number of characters sent, -1 on error
ssize_t serialWrite(char *data, size_t length) {
    return write(fd, data, length);
}

// Read from port. Return number of characters read, 0 if none available, -1 on error
ssize_t serialRead(char *data, size_t length) {
    ssize_t temp = read(fd, data, length);
    if ((temp == -1) && (errno == EAGAIN)) {
        return 0;
    } else {
        return temp;
    }
}

int serialSync(void) {
    return tcdrain(fd);
}

// Close the serial Port
void serialClose(void) {
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
    // Fix every string, addin /dev/ in front of it...
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

char** getSerialPorts() {
    int size;
    char** files = namesInDev(&size);
    char **fin = NULL, **finish = NULL;
    int i = 0, j = 0, f, g;

    // printf("JNI: Got files in /dev (%d)\n", size);

    fin = (char **)malloc(size * sizeof(char *));
    // Has space for all files in dev!

    while (files[i] != NULL) {
        // Filter for SEARCH and if it is a serial port
        if (strstr(files[i], SEARCH) != NULL) {
            // We have a match
            // printf("JNI: %s matched %s", files[i], search);

            // Don't actually check if it is a serial port
            // It causes long delays while trying to connect
            // to Bluetooth devices...

            // f = serialOpen(files[i]);
            // if (f != -1) {
            // printf(" and is a serial port\n");
            fin[j++] = files[i];
            // 	serialClose();
            // } else {
            // printf(" and is not a serial port\n");
            // 	free(files[i]);
            // }


        } else {
            free(files[i]);
        }
        i++;
    }
    free(files);

    // printf("JNI: Found %d serial ports\n", j);

    // Copy in memory with matching size, NULL at end
    finish = (char **)malloc((j + 1) * sizeof(char *));
    finish[j] = NULL;
    for (i = 0; i < j; i++) {
        finish[i] = fin[i];
    }

    free(fin);

    return finish;
}
