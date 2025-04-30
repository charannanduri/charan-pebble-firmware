/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#if defined(MICRO_FAMILY_STM32F2)
#include "stm32f2xx_flash.h"
#elif defined(MICRO_FAMILY_STM32F4)
#include "stm32f4xx_flash.h"
#endif

#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */

// stm32f2xx only has 512k of system flash, these sectors don't exist
#if defined(MICRO_FAMILY_STM32F4)
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */
#endif

typedef void (*SystemFlashProgressCb)(
    uint32_t progress, uint32_t total, void *context);

// Erase the sectors of flash which lie within the given address range.
//
// If the address range overlaps even one single byte of a sector, the entire
// sector is erased.
//
// If progress_callback is not NULL, it is called at the beginning of the erase
// process and after each sector is erased. The rational number (progress/total)
// increases monotonically as the sector erasue procedure progresses.
//
// Returns true if successful, false if an error occurred.
bool system_flash_erase(
    uint32_t address, size_t length,
    SystemFlashProgressCb progress_callback, void *progress_context);

// Write data into flash. The flash must already be erased.
//
// If progress_callback is not NULL, it is called at the beginning of the
// writing process and periodically thereafter. The rational number
// (progress/total) increases monotonically as the data is written.
//
// Returns true if successful, false if an error occurred.
bool system_flash_write(
    uint32_t address, const void *data, size_t length,
    SystemFlashProgressCb progress_callback, void *progress_context);

uint32_t system_flash_read(uint32_t address);
