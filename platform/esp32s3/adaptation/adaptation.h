#ifndef ADAPTATION_H
#define ADAPTATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Hardware initialization
void adaptation_init(void);

// Display interface
void display_init(void);
void display_update(void);
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