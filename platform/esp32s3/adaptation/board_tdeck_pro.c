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

#include "board_tdeck_pro.h"
#include "adaptation.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include <string.h>

static const char *TAG = "Board_TDeckPro";

// SPI device handle for display
static spi_device_handle_t display_spi_handle;

// I2C device handle for keyboard and trackball
static i2c_port_t keyboard_i2c_port;

// LED strip handle
static led_strip_handle_t led_strip;

// Button callback functions
static void (*button_callbacks[BUTTON_ID_MAX])(void) = {NULL};

// Initialize the T-Deck Pro board
void board_tdeck_pro_init(void) {
    ESP_LOGI(TAG, "Initializing T-Deck Pro board");
    
    // Initialize display
    board_tdeck_pro_display_init();
    
    // Initialize keyboard
    board_tdeck_pro_keyboard_init();
    
    // Initialize trackball
    board_tdeck_pro_trackball_init();
    
    // Initialize buttons
    board_tdeck_pro_buttons_init();
    
    // Initialize LED
    board_tdeck_pro_led_init();
    
    // Initialize battery monitoring
    board_tdeck_pro_battery_init();
    
    ESP_LOGI(TAG, "T-Deck Pro board initialization complete");
}

// Initialize the display
void board_tdeck_pro_display_init(void) {
    ESP_LOGI(TAG, "Initializing T-Deck Pro display");
    
    // Initialize GPIO pins for display
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << TDECK_DISPLAY_DC_PIN) | 
                        (1ULL << TDECK_DISPLAY_RST_PIN) | 
                        (1ULL << TDECK_DISPLAY_BL_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);
    
    // Initialize SPI bus for display
    spi_bus_config_t buscfg = {
        .mosi_io_num = TDECK_DISPLAY_SPI_MOSI,
        .miso_io_num = -1,  // No MISO for display
        .sclk_io_num = TDECK_DISPLAY_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TDECK_DISPLAY_WIDTH * TDECK_DISPLAY_HEIGHT * 2 + 8,
    };
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,  // 40 MHz
        .mode = 0,                           // SPI mode 0
        .spics_io_num = TDECK_DISPLAY_SPI_CS,// CS pin
        .queue_size = 7,                     // Queue size
        .pre_cb = NULL,                      // No callback
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(TDECK_DISPLAY_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(TDECK_DISPLAY_SPI_HOST, &devcfg, &display_spi_handle));
    
    // Reset display
    gpio_set_level(TDECK_DISPLAY_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(TDECK_DISPLAY_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Initialize display controller (ST7789V)
    // ST7789V initialization sequence
    uint8_t cmd;
    uint8_t data[16];
    
    // Hardware reset
    gpio_set_level(TDECK_DISPLAY_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TDECK_DISPLAY_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(TDECK_DISPLAY_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // Software reset
    cmd = 0x01; // SWRESET
    gpio_set_level(TDECK_DISPLAY_DC_PIN, 0);
    spi_device_transmit(display_spi_handle, &(spi_transaction_t){
        .length = 8,
        .tx_buffer = &cmd,
    });
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // Sleep out
    cmd = 0x11; // SLPOUT
    gpio_set_level(TDECK_DISPLAY_DC_PIN, 0);
    spi_device_transmit(display_spi_handle, &(spi_transaction_t){
        .length = 8,
        .tx_buffer = &cmd,
    });
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // Set color mode to 16-bit (RGB565)
    cmd = 0x3A; // COLMOD
    gpio_set_level(TDECK_DISPLAY_DC_PIN, 0);
    spi_device_transmit(display_spi_handle, &(spi_transaction_t){
        .length = 8,
        .tx_buffer = &cmd,
    });
    data[0] = 0x05; // 16-bit/pixel
    gpio_set_level(TDECK_DISPLAY_DC_PIN, 1);
    spi_device_transmit(display_spi_handle, &(spi_transaction_t){
        .length = 8,
        .tx_buffer = data,
    });
    
    // Memory data access control
    cmd = 0x36; // MADCTL
    gpio_set_level(TDECK_DISPLAY_DC_PIN, 0);
    spi_device_transmit(display_spi_handle, &(spi_transaction_t){
        .length = 8,
        .tx_buffer = &cmd,
    });
    data[0] = 0x70; // MY=0, MX=1, MV=1, ML=1, RGB=0, MH=0
    gpio_set_level(TDECK_DISPLAY_DC_PIN, 1);
    spi_device_transmit(display_spi_handle, &(spi_transaction_t){
        .length = 8,
        .tx_buffer = data,
    });
    
    // Display inversion on
    cmd = 0x21; // INVON
    gpio_set_level(TDECK_DISPLAY_DC_PIN, 0);
    spi_device_transmit(display_spi_handle, &(spi_transaction_t){
        .length = 8,
        .tx_buffer = &cmd,
    });
    
    // Display on
    cmd = 0x29; // DISPON
    gpio_set_level(TDECK_DISPLAY_DC_PIN, 0);
    spi_device_transmit(display_spi_handle, &(spi_transaction_t){
        .length = 8,
        .tx_buffer = &cmd,
    });
    vTaskDelay(pdMS_TO_TICKS(20));
    
    // Turn on backlight
    gpio_set_level(TDECK_DISPLAY_BL_PIN, 1);
    
    ESP_LOGI(TAG, "T-Deck Pro display initialized");
}

// Initialize the keyboard
void board_tdeck_pro_keyboard_init(void) {
    ESP_LOGI(TAG, "Initializing T-Deck Pro keyboard");
    
    // Initialize I2C for keyboard
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = TDECK_KEYBOARD_I2C_SDA,
        .scl_io_num = TDECK_KEYBOARD_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,  // 400 KHz
    };
    
    ESP_ERROR_CHECK(i2c_param_config(TDECK_KEYBOARD_I2C_HOST, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(TDECK_KEYBOARD_I2C_HOST, I2C_MODE_MASTER, 0, 0, 0));
    
    keyboard_i2c_port = TDECK_KEYBOARD_I2C_HOST;
    
    // TODO: Initialize keyboard controller
    
    ESP_LOGI(TAG, "T-Deck Pro keyboard initialized");
}

// Initialize the trackball
void board_tdeck_pro_trackball_init(void) {
    ESP_LOGI(TAG, "Initializing T-Deck Pro trackball");
    
    // Trackball uses the same I2C bus as the keyboard, so no need to initialize I2C again
    
    // TODO: Initialize trackball controller
    
    ESP_LOGI(TAG, "T-Deck Pro trackball initialized");
}

// Initialize the buttons
void board_tdeck_pro_buttons_init(void) {
    ESP_LOGI(TAG, "Initializing T-Deck Pro buttons");
    
    // Initialize GPIO pins for buttons
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,  // No interrupts for now
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << TDECK_BUTTON_UP_PIN) | 
                        (1ULL << TDECK_BUTTON_SELECT_PIN) | 
                        (1ULL << TDECK_BUTTON_DOWN_PIN) | 
                        (1ULL << TDECK_BUTTON_BACK_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
    
    ESP_LOGI(TAG, "T-Deck Pro buttons initialized");
}

// Initialize the LED
void board_tdeck_pro_led_init(void) {
    ESP_LOGI(TAG, "Initializing T-Deck Pro LED");
    
    // Initialize LED strip
    led_strip_config_t strip_config = {
        .strip_gpio_num = TDECK_LED_PIN,
        .max_leds = 1,  // Only one LED
        .flags.invert_out = false,
    };
    
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,  // 10 MHz
        .mem_block_symbols = 64,
        .flags.with_dma = false,
    };
    
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
    
    ESP_LOGI(TAG, "T-Deck Pro LED initialized");
}

// Initialize the battery monitoring
void board_tdeck_pro_battery_init(void) {
    ESP_LOGI(TAG, "Initializing T-Deck Pro battery monitoring");
    
    // Initialize ADC for battery monitoring
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(TDECK_BATTERY_ADC_CHANNEL, ADC_ATTEN_DB_11);
    
    ESP_LOGI(TAG, "T-Deck Pro battery monitoring initialized");
}

// Set LED color
void board_tdeck_pro_led_set_color(uint8_t r, uint8_t g, uint8_t b) {
    if (led_strip) {
        led_strip_set_pixel(led_strip, 0, r, g, b);
        led_strip_refresh(led_strip);
    }
}

// Check if a button is pressed
bool board_tdeck_pro_button_is_pressed(uint8_t button_id) {
    gpio_num_t pin;
    
    switch (button_id) {
        case BUTTON_ID_UP:
            pin = TDECK_BUTTON_UP_PIN;
            break;
        case BUTTON_ID_SELECT:
            pin = TDECK_BUTTON_SELECT_PIN;
            break;
        case BUTTON_ID_DOWN:
            pin = TDECK_BUTTON_DOWN_PIN;
            break;
        case BUTTON_ID_BACK:
            pin = TDECK_BUTTON_BACK_PIN;
            break;
        default:
            return false;
    }
    
    return !gpio_get_level(pin);  // Buttons are active low
}

// Set button callback
void board_tdeck_pro_button_set_callback(uint8_t button_id, void (*callback)(void)) {
    if (button_id < BUTTON_ID_MAX) {
        button_callbacks[button_id] = callback;
    }
}

// Get battery level (0-100)
uint8_t board_tdeck_pro_battery_get_level(void) {
    // Read ADC value
    int adc_value = adc1_get_raw(TDECK_BATTERY_ADC_CHANNEL);
    
    // Convert ADC value to battery level (0-100)
    // TODO: Calibrate this conversion based on actual battery characteristics
    uint8_t level = (adc_value * 100) / 4095;
    
    return level;
}

// Read keyboard input
bool board_tdeck_pro_keyboard_read(uint8_t *keycode) {
    // TODO: Implement keyboard reading
    return false;
}

// Read trackball input
bool board_tdeck_pro_trackball_read(int8_t *dx, int8_t *dy) {
    // TODO: Implement trackball reading
    return false;
}
