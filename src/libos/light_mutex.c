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

#include "light_mutex.h"
#include "os/assert.h"

#ifdef PLATFORM_ESP32S3
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#endif

// Implementation of light mutex functions for ESP32-S3

void light_mutex_init(LightMutex* mutex) {
    mutex->handle = xSemaphoreCreateMutex();
    mutex->initialized = (mutex->handle != NULL);
}

void light_mutex_lock(LightMutex* mutex) {
    if (!mutex->initialized) {
        // Auto-initialize if not already initialized
        light_mutex_init(mutex);
    }
    xSemaphoreTake(mutex->handle, portMAX_DELAY);
}

void light_mutex_unlock(LightMutex* mutex) {
    if (mutex->initialized) {
        xSemaphoreGive(mutex->handle);
    }
}

bool light_mutex_try_lock(LightMutex* mutex) {
    if (!mutex->initialized) {
        // Auto-initialize if not already initialized
        light_mutex_init(mutex);
    }
    return xSemaphoreTake(mutex->handle, 0) == pdTRUE;
}

void light_mutex_cleanup(LightMutex* mutex) {
    if (mutex->initialized) {
        vSemaphoreDelete(mutex->handle);
        mutex->initialized = false;
    }
}

// FreeRTOS light mutex handle functions needed by mutex_freertos.c
LightMutexHandle_t xLightMutexCreate(void) {
    LightMutex* mutex = (LightMutex*)pvPortMalloc(sizeof(LightMutex));
    if (mutex) {
        light_mutex_init(mutex);
    }
    return mutex;
}

void vLightMutexDelete(LightMutexHandle_t xMutex) {
    if (xMutex) {
        light_mutex_cleanup((LightMutex*)xMutex);
        vPortFree(xMutex);
    }
}

BaseType_t xLightMutexLock(LightMutexHandle_t xMutex, TickType_t xTicksToWait) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (!mutex->initialized) {
        light_mutex_init(mutex);
    }
    return xSemaphoreTake(mutex->handle, xTicksToWait);
}

void xLightMutexUnlock(LightMutexHandle_t xMutex) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (mutex->initialized) {
        xSemaphoreGive(mutex->handle);
    }
}

// Recursive mutex functions
LightMutexHandle_t xLightMutexCreateRecursive(void) {
    LightMutex* mutex = (LightMutex*)pvPortMalloc(sizeof(LightMutex));
    if (mutex) {
        mutex->handle = xSemaphoreCreateRecursiveMutex();
        mutex->initialized = (mutex->handle != NULL);
    }
    return mutex;
}

BaseType_t xLightMutexLockRecursive(LightMutexHandle_t xMutex, TickType_t xTicksToWait) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (!mutex->initialized) {
        mutex->handle = xSemaphoreCreateRecursiveMutex();
        mutex->initialized = (mutex->handle != NULL);
    }
    return xSemaphoreTakeRecursive(mutex->handle, xTicksToWait);
}

void xLightMutexUnlockRecursive(LightMutexHandle_t xMutex) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (mutex->initialized) {
        xSemaphoreGiveRecursive(mutex->handle);
    }
}

void* xLightMutexGetHolder(LightMutexHandle_t xMutex) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (mutex && mutex->initialized) {
        return xSemaphoreGetMutexHolder(mutex->handle);
    }
    return NULL;
}

UBaseType_t uxLightMutexGetRecursiveCallCount(LightMutexHandle_t xMutex) {
    // This is a bit of a hack since FreeRTOS doesn't expose this directly
    // For ESP32, we'll just return 1 if the mutex is held by the current task
    LightMutex* mutex = (LightMutex*)xMutex;
    if (mutex && mutex->initialized) {
        if (xSemaphoreGetMutexHolder(mutex->handle) == xTaskGetCurrentTaskHandle()) {
            return 1;
        }
    }
    return 0;
}
