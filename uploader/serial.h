/*
 * serial.h
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

// Open the serial port. Return file handle on success, -1 on error.
int serialOpen(char *port, int baud, int flowcontrol);
void serialClose(int fd);

int serialHasChar(int fd); // Returns 1 if char is available, 0 if not.

// Blocking functions
void serialWriteChar(int fd, char c);
void serialReadChar(int fd, char *c);
void serialWriteString(int fd, char *s);

// String array with serial port names. Free after use!
char** getSerialPorts(void);
