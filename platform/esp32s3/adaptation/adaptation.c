#include "adaptation.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "led_strip.h"

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

void adaptation_init(void) {
    ESP_LOGI(TAG, "Initializing adaptation layer");
    
    // Initialize display
    display_init();
    
    // Initialize buttons
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_UP_PIN) |
                       (1ULL << BUTTON_SELECT_PIN) |
                       (1ULL << BUTTON_DOWN_PIN) |
                       (1ULL << BUTTON_BACK_PIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
    
    // Initialize timer
    timer_init();
    
    // Initialize sensors
    sensors_init();
    
    // Initialize power management
    power_init();
    
    // Initialize Bluetooth
    bluetooth_init();

    // Initialize LED
    ESP_LOGI(TAG, "Configuring WS2812 LED strip");
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = 1, // Only one LED on the board
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 64, // Increase if driving more LEDs
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);
    ESP_LOGI(TAG, "WS2812 LED strip configured");
}

// Display interface implementation
void display_init(void) {
    // TODO: Initialize display hardware (SPI, etc.)
    ESP_LOGI(TAG, "Display initialized");
}

void display_update(void) {
    // TODO: Update display with current framebuffer
}

void display_clear(void) {
    // TODO: Clear display
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