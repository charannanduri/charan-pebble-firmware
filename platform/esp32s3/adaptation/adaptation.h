#ifndef ADAPTATION_H
#define ADAPTATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// PebbleOS Logging interface
typedef enum {
    PEBBLE_LOG_LEVEL_ERROR,
    PEBBLE_LOG_LEVEL_WARNING,
    PEBBLE_LOG_LEVEL_INFO,
    PEBBLE_LOG_LEVEL_DEBUG,
    PEBBLE_LOG_LEVEL_VERBOSE
} pebble_log_level_t;

void pebble_log_message(pebble_log_level_t level, const char *tag, const char *format, ...);

// Hardware initialization
void adaptation_init(void);

// Display interface
// Forward declare DisplayRow - Needed by callback types defined in Pebble headers
struct DisplayRow;

// Define callback types matching PebbleOS (REMOVED - Use definitions from Pebble headers)
// typedef bool (*NextRowCallback)(struct DisplayRow* row);
// typedef void (*UpdateCompleteCallback)(void);

// Define function pointer types explicitly for the function declaration
typedef bool (*AdaptationNextRowCallback)(struct DisplayRow* row);
typedef void (*AdaptationUpdateCompleteCallback)(void);

void display_init(void);
// Update signature to use the explicit types (or assume Pebble types are in scope)
void adaptation_display_update(AdaptationNextRowCallback nrcb, AdaptationUpdateCompleteCallback uccb);
void display_clear(void);

// Button interface
typedef enum {
    BUTTON_UP,
    BUTTON_SELECT,
    BUTTON_DOWN,
    BUTTON_BACK
} Button;

bool button_is_pressed(Button button);
void button_set_callback(Button button, void (*callback)(void));

// Timer interface
typedef void (*TimerCallback)(void);
void timer_init(void);
void timer_start(uint32_t ms, TimerCallback callback);
void timer_stop(void);

// Memory management
void* adaptation_malloc(size_t size);
void adaptation_free(void* ptr);

// Power management
void power_init(void);
void power_enter_sleep(void);
void power_exit_sleep(void);

// Bluetooth interface
void bluetooth_init(void);
void bluetooth_start_advertising(void);
void bluetooth_stop_advertising(void);

// Sensor interface
void sensors_init(void);
bool sensors_get_accel_data(int16_t* x, int16_t* y, int16_t* z);
bool sensors_get_mag_data(int16_t* x, int16_t* y, int16_t* z);

// LED Interface (WS2812)
void led_set_color(uint8_t r, uint8_t g, uint8_t b);

#endif // ADAPTATION_H 