/*
 * spm.c
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
#include <avr/io.h>
#include <stdint.h>
#include <avr/boot.h>
#include <util/atomic.h>

#include "global.h"

void program(uint32_t page, uint8_t *d) {
	uint16_t i, w;
	PROGRAMMED();
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		eeprom_busy_wait();
		boot_page_erase(page);
		boot_spm_busy_wait();
		for (i = 0; i < SPM_PAGESIZE; i += 2) {
			w = *d++;
			w += ((*d++) << 8);
			boot_page_fill(page + i, w);
		}
		boot_page_write(page);
		boot_spm_busy_wait();
		boot_rww_enable(); // Allows us to jump back
	}
}
