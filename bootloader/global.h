/*
 * global.h
 *
 * Copyright 2012 Thomas Buck <xythobuz@me.com>
 *
 * This file is part of YASAB.
 *
 * YASAB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YASAB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with YASAB.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "serial.h"

// Debug Output
#if defined(DEBUG) && (DEBUG > 0)
#define debugPrint(x) serialWriteString(x)
#else
#define debugPrint(ignore)
#endif

#define ERROR 'e'
#define OKAY 'o'
#define FLASH 'f'
#define CONFIRM 'c'

extern uint8_t buf[SPM_PAGESIZE];

void parse(void);
void set(uint8_t *d, uint8_t c, uint16_t l);
void program(uint32_t page, uint8_t *d);
void gotoApplication(void);
