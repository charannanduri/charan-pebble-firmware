#include "adaptation.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "led_strip.h"
#include <stdarg.h> // Needed for va_list

static const char *TAG = "Adaptation";

// LED configuration
#define LED_GPIO GPIO_NUM_21
static led_strip_handle_t led_strip;

// Button configuration
#define BUTTON_UP_PIN     GPIO_NUM_0
#define BUTTON_SELECT_PIN GPIO_NUM_1
#define BUTTON_DOWN_PIN   GPIO_NUM_2
#define BUTTON_BACK_PIN   GPIO_NUM_3

// Button callback functions
static void (*button_callbacks[4])(void) = {NULL, NULL, NULL, NULL};

// Timer handle
static esp_timer_handle_t timer_handle = NULL;

// --- Logging ---

void pebble_log_message(pebble_log_level_t level, const char *tag, const char *format, ...) {
    va_list args;
    va_start(args, format);

    // Map Pebble log level to ESP log level
    esp_log_level_t esp_level;
    switch (level) {
        case PEBBLE_LOG_LEVEL_ERROR:   esp_level = ESP_LOG_ERROR;   break;
        case PEBBLE_LOG_LEVEL_WARNING: esp_level = ESP_LOG_WARN;    break;
        case PEBBLE_LOG_LEVEL_INFO:    esp_level = ESP_LOG_INFO;    break;
        case PEBBLE_LOG_LEVEL_DEBUG:   esp_level = ESP_LOG_DEBUG;   break;
        case PEBBLE_LOG_LEVEL_VERBOSE: esp_level = ESP_LOG_VERBOSE; break;
        default:                       esp_level = ESP_LOG_INFO;    break;
    }

    // Use ESP-IDF's vprintf logging
    esp_log_writev(esp_level, tag ? tag : "Pebble", format, args); // Use default tag if NULL

    va_end(args);
}

// --- Initialization ---

void adaptation_init(void) {
    ESP_LOGI(TAG, "Initializing adaptation layer");
    
    // Initialize the T-Deck Pro board
    // This will initialize display, keyboard, trackball, buttons, LED, and battery
    board_tdeck_pro_init();
    
    // Initialize timer
    timer_init();
    
    // Initialize sensors
    sensors_init();
    
    // Initialize power management
    power_init();
    
    // Initialize Bluetooth
    bluetooth_init();

    ESP_LOGI(TAG, "Adaptation layer initialized");
}

// Display interface implementation is now in display_esp32s3.c
// These functions are kept here as stubs for backward compatibility
// and will be removed in the future.
void display_init(void) {
    // Implemented in display_esp32s3.c
}

void adaptation_display_update(AdaptationNextRowCallback nrcb, AdaptationUpdateCompleteCallback uccb) {
    // Implemented in display_esp32s3.c
}

void display_clear(void) {
    // Implemented in display_esp32s3.c
}

// Button interface implementation
bool button_is_pressed(Button button) {
    gpio_num_t pin;
    switch(button) {
        case BUTTON_UP:     pin = BUTTON_UP_PIN;     break;
        case BUTTON_SELECT: pin = BUTTON_SELECT_PIN; break;
        case BUTTON_DOWN:   pin = BUTTON_DOWN_PIN;   break;
        case BUTTON_BACK:   pin = BUTTON_BACK_PIN;   break;
        default: return false;
    }
    return !gpio_get_level(pin); // Active low
}

void button_set_callback(Button button, void (*callback)(void)) {
    if (button >= 0 && button < 4) {
        button_callbacks[button] = callback;
    }
}

// Timer interface implementation
static void timer_callback(void* arg) {
    TimerCallback callback = (TimerCallback)arg;
    if (callback) {
        callback();
    }
}

void timer_init(void) {
    // Timer is already initialized by ESP-IDF
}

void timer_start(uint32_t ms, TimerCallback callback) {
    if (timer_handle) {
        esp_timer_stop(timer_handle);
        esp_timer_delete(timer_handle);
    }
    
    const esp_timer_create_args_t timer_args = {
        .callback = timer_callback,
        .arg = (void*)callback,
        .name = "pebble_timer"
    };
    
    esp_timer_create(&timer_args, &timer_handle);
    esp_timer_start_periodic(timer_handle, ms * 1000);
}

void timer_stop(void) {
    if (timer_handle) {
        esp_timer_stop(timer_handle);
        esp_timer_delete(timer_handle);
        timer_handle = NULL;
    }
}

// Memory management implementation
void* adaptation_malloc(size_t size) {
    return heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
}

void adaptation_free(void* ptr) {
    heap_caps_free(ptr);
}

// Power management implementation
void power_init(void) {
    // TODO: Initialize power management
}

void power_enter_sleep(void) {
    // TODO: Implement sleep mode
}

void power_exit_sleep(void) {
    // TODO: Implement wake from sleep
}

// Bluetooth interface implementation
void bluetooth_init(void) {
    // TODO: Initialize Bluetooth
}

void bluetooth_start_advertising(void) {
    // TODO: Start Bluetooth advertising
}

void bluetooth_stop_advertising(void) {
    // TODO: Stop Bluetooth advertising
}

// Sensor interface implementation
void sensors_init(void) {
    // TODO: Initialize sensors
}

bool sensors_get_accel_data(int16_t* x, int16_t* y, int16_t* z) {
    // TODO: Implement accelerometer reading
    return false;
}

bool sensors_get_mag_data(int16_t* x, int16_t* y, int16_t* z) {
    // TODO: Implement magnetometer reading
    return false;
}

// LED Interface Implementation
void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
    if (led_strip) {
        led_strip_set_pixel(led_strip, 0, r, g, b);
        led_strip_refresh(led_strip);
    }
}
