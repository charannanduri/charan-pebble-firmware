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

#include <stdint.h>
#include <stdbool.h>

// LilyGo T-Deck Pro board definition
// Based on https://github.com/Xinyuan-LilyGO/T-Deck-Pro

// Display configuration
// T-Deck Pro uses a 2.8" 320x240 LCD with ST7789V controller
#define TDECK_DISPLAY_WIDTH  320
#define TDECK_DISPLAY_HEIGHT 240

// Display pins - Updated with values from T-Deck Pro GitHub repository
#define TDECK_DISPLAY_SPI_HOST    SPI2_HOST
#define TDECK_DISPLAY_SPI_MOSI    GPIO_NUM_33  // BOARD_SPI_MOSI from test_EPD.ino
#define TDECK_DISPLAY_SPI_SCLK    GPIO_NUM_36  // BOARD_SPI_SCK from test_EPD.ino
#define TDECK_DISPLAY_SPI_CS      GPIO_NUM_34  // BOARD_SPI_CS from test_EPD.ino
#define TDECK_DISPLAY_DC_PIN      GPIO_NUM_35  // BOARD_SPI_DC from test_EPD.ino
#define TDECK_DISPLAY_RST_PIN     GPIO_NUM_21  // Kept as placeholder, not defined in test_EPD.ino
#define TDECK_DISPLAY_BL_PIN      GPIO_NUM_38  // Kept as placeholder, not defined in test_EPD.ino

// Keyboard configuration
// T-Deck Pro has a built-in QWERTY keyboard
#define TDECK_KEYBOARD_I2C_HOST   I2C_NUM_0
#define TDECK_KEYBOARD_I2C_SDA    GPIO_NUM_10
#define TDECK_KEYBOARD_I2C_SCL    GPIO_NUM_11
#define TDECK_KEYBOARD_I2C_ADDR   0x55  // Placeholder, update with actual address

// Trackball configuration
// T-Deck Pro has a trackball for navigation
#define TDECK_TRACKBALL_I2C_HOST  I2C_NUM_0  // Same as keyboard
#define TDECK_TRACKBALL_I2C_SDA   GPIO_NUM_10 // Same as keyboard
#define TDECK_TRACKBALL_I2C_SCL   GPIO_NUM_11 // Same as keyboard
#define TDECK_TRACKBALL_I2C_ADDR  0x56  // Placeholder, update with actual address

// Button configuration
// T-Deck Pro has several physical buttons
#define TDECK_BUTTON_UP_PIN       GPIO_NUM_0  // Placeholder
#define TDECK_BUTTON_SELECT_PIN   GPIO_NUM_1  // Placeholder
#define TDECK_BUTTON_DOWN_PIN     GPIO_NUM_2  // Placeholder
#define TDECK_BUTTON_BACK_PIN     GPIO_NUM_3  // Placeholder

// LED configuration
#define TDECK_LED_PIN             GPIO_NUM_46  // Placeholder

// Battery configuration
#define TDECK_BATTERY_ADC_CHANNEL ADC1_CHANNEL_0  // Placeholder

// Define button IDs (ensure these match Pebble OS expectations)
#define BUTTON_ID_BACK   0
#define BUTTON_ID_UP     1
#define BUTTON_ID_SELECT 2
#define BUTTON_ID_DOWN   3
#define BUTTON_ID_MAX    4 // Number of buttons

// Display dimensions for PebbleOS
#define PBL_DISPLAY_WIDTH  TDECK_DISPLAY_WIDTH
#define PBL_DISPLAY_HEIGHT TDECK_DISPLAY_HEIGHT

// Function declarations
void board_tdeck_pro_init(void);
void board_tdeck_pro_display_init(void);
void board_tdeck_pro_keyboard_init(void);
void board_tdeck_pro_trackball_init(void);
void board_tdeck_pro_buttons_init(void);
void board_tdeck_pro_led_init(void);
void board_tdeck_pro_battery_init(void);
