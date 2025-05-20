/*
 * Minimal main entry point for ESP-IDF.
 * This will eventually be replaced or augmented by the PebbleOS entry point
 * and the adaptation layer.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "adaptation.h"
#include <stdint.h>
#include "../../../src/fw/kernel/kernel_applib_state.h"

// Declare kernel_applib_init (assuming it needs to be called explicitly)
// We need to find the correct header file for this.
// For now, declare it manually to see if it links.
// void kernel_applib_init(void);

static const char *TAG = "PebbleMain";

// Helper function to convert HSV to RGB
// H = 0-359, S = 0-255, V = 0-255
void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
    uint8_t region, remainder, p, q, t;

    if (s == 0) {
        *r = v;
        *g = v;
        *b = v;
        return;
    }

    region = h / 60;
    remainder = (h % 60) * 255 / 60;

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0:
            *r = v; *g = t; *b = p;
            break;
        case 1:
            *r = q; *g = v; *b = p;
            break;
        case 2:
            *r = p; *g = v; *b = t;
            break;
        case 3:
            *r = p; *g = q; *b = v;
            break;
        case 4:
            *r = t; *g = p; *b = v;
            break;
        default:
            *r = v; *g = p; *b = q;
            break;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP-IDF Pebble Base Project Starting!");

    // Initialize the adaptation layer (sets up drivers, logging, etc.)
    adaptation_init();

    // Initialize PebbleOS core application services
    // This function was found in src/fw/kernel/kernel_applib_state.c
    ESP_LOGI(TAG, "Initializing PebbleOS Kernel Applib...");
    kernel_applib_init(); 

    ESP_LOGI(TAG, "Initialization complete. PebbleOS should take over via its own tasks.");

    // // Keep the main task alive and cycle LED color (REMOVED - PebbleOS should control)
    // while(1) {
    //     // Convert current hue to RGB
    //     hsv_to_rgb(hue, saturation, value, &red, &green, &blue);
    //     
    //     // Set the LED color (uses adaptation layer)
    //     led_set_color(red, green, blue);
    // 
    //     // Increment hue, wrap around at 360
    //     hue++;
    //     if (hue >= 360) {
    //         hue = 0;
    //     }
    // 
    //     // Delay for smooth transition
    //     vTaskDelay(pdMS_TO_TICKS(20)); // Adjust delay for fade speed
    // }

    // Let the main task finish or idle. The FreeRTOS scheduler will run other tasks.
    // For safety, we can make it sleep indefinitely.
    ESP_LOGI(TAG, "app_main yielding control.");
    vTaskSuspend(NULL); // Suspend this task indefinitely
} 