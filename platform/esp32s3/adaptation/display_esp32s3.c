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

#include "display_esp32s3.h"
#include "adaptation.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "Display_ESP32S3";

// Display configuration
#define DISPLAY_SPI_HOST    SPI2_HOST
#define DISPLAY_SPI_MOSI    GPIO_NUM_35
#define DISPLAY_SPI_SCLK    GPIO_NUM_36
#define DISPLAY_SPI_CS      GPIO_NUM_37
#define DISPLAY_DC_PIN      GPIO_NUM_38
#define DISPLAY_RST_PIN     GPIO_NUM_39
#define DISPLAY_BL_PIN      GPIO_NUM_40

// Display dimensions (defined in board_chameleon.h)
#define DISPLAY_WIDTH       240
#define DISPLAY_HEIGHT      240

// Display commands
#define DISPLAY_CMD_NOP     0x00
#define DISPLAY_CMD_SWRESET 0x01
#define DISPLAY_CMD_SLPIN   0x10
#define DISPLAY_CMD_SLPOUT  0x11
#define DISPLAY_CMD_INVOFF  0x20
#define DISPLAY_CMD_INVON   0x21
#define DISPLAY_CMD_DISPOFF 0x28
#define DISPLAY_CMD_DISPON  0x29
#define DISPLAY_CMD_CASET   0x2A
#define DISPLAY_CMD_RASET   0x2B
#define DISPLAY_CMD_RAMWR   0x2C
#define DISPLAY_CMD_MADCTL  0x36
#define DISPLAY_CMD_COLMOD  0x3A

// Display color format
#define DISPLAY_COLOR_16BIT 0x05
#define DISPLAY_COLOR_18BIT 0x06

// Display orientation
#define DISPLAY_MADCTL_MY   0x80
#define DISPLAY_MADCTL_MX   0x40
#define DISPLAY_MADCTL_MV   0x20
#define DISPLAY_MADCTL_ML   0x10
#define DISPLAY_MADCTL_RGB  0x00
#define DISPLAY_MADCTL_BGR  0x08

// Display SPI device handle
static spi_device_handle_t spi_handle;

// Display framebuffer
static uint16_t framebuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT];

// Display update callback
static AdaptationUpdateCompleteCallback update_complete_callback = NULL;

// Send command to display
static void display_cmd(uint8_t cmd) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    gpio_set_level(DISPLAY_DC_PIN, 0); // Command mode
    ret = spi_device_polling_transmit(spi_handle, &t);
    assert(ret == ESP_OK);
}

// Send data to display
static void display_data(const uint8_t *data, int len) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8;
    t.tx_buffer = data;
    gpio_set_level(DISPLAY_DC_PIN, 1); // Data mode
    ret = spi_device_polling_transmit(spi_handle, &t);
    assert(ret == ESP_OK);
}

// Send 16-bit data to display
static void display_data16(uint16_t data) {
    uint8_t buffer[2];
    buffer[0] = (data >> 8) & 0xFF;
    buffer[1] = data & 0xFF;
    display_data(buffer, 2);
}

// Set display address window
static void display_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    display_cmd(DISPLAY_CMD_CASET);
    display_data16(x0);
    display_data16(x1);
    display_cmd(DISPLAY_CMD_RASET);
    display_data16(y0);
    display_data16(y1);
    display_cmd(DISPLAY_CMD_RAMWR);
}

// Initialize display
void display_init(void) {
    ESP_LOGI(TAG, "Initializing display");

    // Initialize GPIO pins
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << DISPLAY_DC_PIN) | (1ULL << DISPLAY_RST_PIN) | (1ULL << DISPLAY_BL_PIN),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_conf);

    // Initialize SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = DISPLAY_SPI_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = DISPLAY_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2 + 8,
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,  // 10 MHz
        .mode = 0,                           // SPI mode 0
        .spics_io_num = DISPLAY_SPI_CS,      // CS pin
        .queue_size = 7,                     // We want to be able to queue 7 transactions at a time
        .pre_cb = NULL,                      // No callback
    };
    ESP_ERROR_CHECK(spi_bus_initialize(DISPLAY_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(DISPLAY_SPI_HOST, &devcfg, &spi_handle));

    // Reset display
    gpio_set_level(DISPLAY_RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(DISPLAY_RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Initialize display
    display_cmd(DISPLAY_CMD_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    display_cmd(DISPLAY_CMD_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(500));
    display_cmd(DISPLAY_CMD_COLMOD);
    uint8_t data = DISPLAY_COLOR_16BIT;
    display_data(&data, 1);
    display_cmd(DISPLAY_CMD_MADCTL);
    data = DISPLAY_MADCTL_RGB;
    display_data(&data, 1);
    display_cmd(DISPLAY_CMD_INVON);
    display_cmd(DISPLAY_CMD_DISPON);

    // Turn on backlight
    gpio_set_level(DISPLAY_BL_PIN, 1);

    // Clear display
    display_clear();

    ESP_LOGI(TAG, "Display initialized");
}

// Clear display
void display_clear(void) {
    ESP_LOGI(TAG, "Clearing display");
    memset(framebuffer, 0, sizeof(framebuffer));
    display_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    display_data((uint8_t *)framebuffer, DISPLAY_WIDTH * DISPLAY_HEIGHT * 2);
}

// Update display with data from PebbleOS
void adaptation_display_update(AdaptationNextRowCallback nrcb, AdaptationUpdateCompleteCallback uccb) {
    ESP_LOGI(TAG, "Updating display");

    // Save callback
    update_complete_callback = uccb;

    // Get display data from PebbleOS
    struct DisplayRow row;
    uint16_t row_index = 0;
    while (nrcb(&row)) {
        // Copy row data to framebuffer
        memcpy(&framebuffer[row.row_index * DISPLAY_WIDTH], row.data, DISPLAY_WIDTH * 2);
        row_index++;
    }

    // Update display
    display_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
    display_data((uint8_t *)framebuffer, DISPLAY_WIDTH * DISPLAY_HEIGHT * 2);

    // Call update complete callback
    if (update_complete_callback) {
        update_complete_callback();
    }
}
