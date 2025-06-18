#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef PLATFORM_ESP32S3
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#endif

// Light mutex implementation for PebbleOS on ESP32S3
typedef struct {
    SemaphoreHandle_t handle;
    bool initialized;
} LightMutex;

// Handle type for light mutex
typedef LightMutex* LightMutexHandle_t;

// Initialize a light mutex
void light_mutex_init(LightMutex* mutex);

// Lock a light mutex
void light_mutex_lock(LightMutex* mutex);

// Unlock a light mutex  
void light_mutex_unlock(LightMutex* mutex);

// Try to lock a light mutex (non-blocking)
bool light_mutex_try_lock(LightMutex* mutex);

// Cleanup a light mutex
void light_mutex_cleanup(LightMutex* mutex);
