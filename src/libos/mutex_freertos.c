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

#include "mcu/interrupts.h"
#include "os/assert.h"
#include "os/malloc.h"
#include "os/mutex.h"
#include "os/tick.h"
#include "os/os_common.h"

#ifdef PLATFORM_ESP32S3
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#else
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#endif

// Light mutex implementation for ESP32-S3
#ifdef PLATFORM_ESP32S3
// Implementation of light mutex functions for ESP32-S3
typedef struct {
    SemaphoreHandle_t handle;
    bool initialized;
} LightMutex;

// Handle type for light mutex
typedef LightMutex* LightMutexHandle_t;

// Initialize a light mutex
static void light_mutex_init(LightMutex* mutex) {
    mutex->handle = xSemaphoreCreateMutex();
    mutex->initialized = (mutex->handle != NULL);
}

// Lock a light mutex
static void light_mutex_lock(LightMutex* mutex) {
    if (!mutex->initialized) {
        // Auto-initialize if not already initialized
        light_mutex_init(mutex);
    }
    xSemaphoreTake(mutex->handle, portMAX_DELAY);
}

// Unlock a light mutex  
static void light_mutex_unlock(LightMutex* mutex) {
    if (mutex->initialized) {
        xSemaphoreGive(mutex->handle);
    }
}

// Try to lock a light mutex (non-blocking)
static bool light_mutex_try_lock(LightMutex* mutex) {
    if (!mutex->initialized) {
        // Auto-initialize if not already initialized
        light_mutex_init(mutex);
    }
    return xSemaphoreTake(mutex->handle, 0) == pdTRUE;
}

// Cleanup a light mutex
static void light_mutex_cleanup(LightMutex* mutex) {
    if (mutex->initialized) {
        vSemaphoreDelete(mutex->handle);
        mutex->initialized = false;
    }
}

// FreeRTOS light mutex handle functions needed by mutex_freertos.c
static LightMutexHandle_t xLightMutexCreate(void) {
    LightMutex* mutex = (LightMutex*)pvPortMalloc(sizeof(LightMutex));
    if (mutex) {
        light_mutex_init(mutex);
    }
    return mutex;
}

static void vLightMutexDelete(LightMutexHandle_t xMutex) {
    if (xMutex) {
        light_mutex_cleanup((LightMutex*)xMutex);
        vPortFree(xMutex);
    }
}

static BaseType_t xLightMutexLock(LightMutexHandle_t xMutex, TickType_t xTicksToWait) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (!mutex->initialized) {
        light_mutex_init(mutex);
    }
    return xSemaphoreTake(mutex->handle, xTicksToWait);
}

static void xLightMutexUnlock(LightMutexHandle_t xMutex) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (mutex->initialized) {
        xSemaphoreGive(mutex->handle);
    }
}

// Recursive mutex functions
static LightMutexHandle_t xLightMutexCreateRecursive(void) {
    LightMutex* mutex = (LightMutex*)pvPortMalloc(sizeof(LightMutex));
    if (mutex) {
        mutex->handle = xSemaphoreCreateRecursiveMutex();
        mutex->initialized = (mutex->handle != NULL);
    }
    return mutex;
}

static BaseType_t xLightMutexLockRecursive(LightMutexHandle_t xMutex, TickType_t xTicksToWait) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (!mutex->initialized) {
        mutex->handle = xSemaphoreCreateRecursiveMutex();
        mutex->initialized = (mutex->handle != NULL);
    }
    return xSemaphoreTakeRecursive(mutex->handle, xTicksToWait);
}

static void xLightMutexUnlockRecursive(LightMutexHandle_t xMutex) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (mutex->initialized) {
        xSemaphoreGiveRecursive(mutex->handle);
    }
}

static void* xLightMutexGetHolder(LightMutexHandle_t xMutex) {
    LightMutex* mutex = (LightMutex*)xMutex;
    if (mutex && mutex->initialized) {
        return xSemaphoreGetMutexHolder(mutex->handle);
    }
    return NULL;
}

static UBaseType_t uxLightMutexGetRecursiveCallCount(LightMutexHandle_t xMutex) {
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
#else
#include "light_mutex.h"
#endif

#include <string.h>

typedef struct {
    uint32_t lr;
    LightMutexHandle_t freertos_mutex;
} PebbleMutexCommon;

struct pebble_mutex_t {
  PebbleMutexCommon common;
};

struct pebble_recursive_mutex_t {
  PebbleMutexCommon common;
};

// macros should only be called while we hold the mutex we are logging so no
// additional locking is needed
#define LOG_LOCKED(logged_lr, new_lr)    \
  if (logged_lr == 0) {              \
    logged_lr = new_lr;              \
  }

#define LOG_UNLOCKED(logged_lr, nest_count) \
  if (nest_count == 1) {                \
    logged_lr = 0;                      \
  }

static PebbleMutexCommon *create_pebble_mutex(LightMutexHandle_t freertos_mutex) {
  PebbleMutexCommon *mutex = os_malloc_check(sizeof(PebbleMutex));
  *mutex = (PebbleMutexCommon) {
    .freertos_mutex = freertos_mutex,
    .lr = 0
  };

  return mutex;
}

PebbleMutex * mutex_create(void) {
  LightMutexHandle_t freertos_mutex = xLightMutexCreate();
  OS_ASSERT(freertos_mutex != NULL);
  PebbleMutex *mutex = (PebbleMutex *)create_pebble_mutex(freertos_mutex);
  return mutex;
}

void mutex_destroy(PebbleMutex * handle) {
  OS_ASSERT(handle != NULL);
  LightMutexHandle_t mutex = handle->common.freertos_mutex;
  vLightMutexDelete(mutex);
  os_free(handle);
}

void mutex_lock(PebbleMutex * handle) {
  uintptr_t myLR = (uintptr_t) __builtin_return_address(0);

  OS_ASSERT(!mcu_state_is_isr());

  xLightMutexLock(handle->common.freertos_mutex, portMAX_DELAY);
  LOG_LOCKED(handle->common.lr, myLR);
}

bool mutex_lock_with_timeout(PebbleMutex * handle, uint32_t timeout_ms) {
  uintptr_t myLR = (uintptr_t) __builtin_return_address(0);

  OS_ASSERT(!mcu_state_is_isr());

  TickType_t timeout_ticks = milliseconds_to_ticks(timeout_ms);
  LightMutexHandle_t mutex = handle->common.freertos_mutex;

  if (xLightMutexLock(mutex, timeout_ticks) == pdTRUE) {
    LOG_LOCKED(handle->common.lr, myLR);
    return (true);
  }

  return (false);
}

void mutex_lock_with_lr(PebbleMutex * handle, uint32_t myLR) {
  OS_ASSERT(!mcu_state_is_isr());

  xLightMutexLock(handle->common.freertos_mutex, portMAX_DELAY);
  LOG_LOCKED(handle->common.lr, myLR);
}

void mutex_unlock(PebbleMutex * handle) {
  OS_ASSERT(!mcu_state_is_isr());
  LOG_UNLOCKED(handle->common.lr, 1)
  xLightMutexUnlock(handle->common.freertos_mutex);
}

static void prv_assert_held_by_curr_task(PebbleMutex * handle, bool is_held, uint32_t lr) {
  LightMutexHandle_t mutex = handle->common.freertos_mutex;
  OS_ASSERT_LR((xLightMutexGetHolder(mutex) == xTaskGetCurrentTaskHandle()) == is_held,
                 lr);
}

void mutex_assert_held_by_curr_task(PebbleMutex * handle, bool is_held) {
  uintptr_t saved_lr = (uintptr_t) __builtin_return_address(0);
  prv_assert_held_by_curr_task(handle, is_held, saved_lr);
}

void mutex_assert_recursive_held_by_curr_task(PebbleRecursiveMutex * handle, bool is_held) {
  uintptr_t saved_lr = (uintptr_t) __builtin_return_address(0);
  prv_assert_held_by_curr_task((PebbleMutex *) handle, is_held, saved_lr);
}

PebbleRecursiveMutex * mutex_create_recursive(void) {
  LightMutexHandle_t freertos_mutex = xLightMutexCreateRecursive();
  OS_ASSERT(freertos_mutex != NULL);
  PebbleRecursiveMutex * mutex = (PebbleRecursiveMutex *)create_pebble_mutex(freertos_mutex);

  return mutex;
}

void mutex_lock_recursive(PebbleRecursiveMutex * handle) {
  uintptr_t myLR = (uintptr_t) __builtin_return_address(0);
  OS_ASSERT(!mcu_state_is_isr());
  xLightMutexLockRecursive(handle->common.freertos_mutex, portMAX_DELAY);
  LOG_LOCKED(handle->common.lr, myLR);
}

bool mutex_lock_recursive_with_timeout_and_lr(PebbleRecursiveMutex * handle,
    uint32_t timeout_ms, uint32_t myLR) {
  OS_ASSERT(!mcu_state_is_isr());

  TickType_t timeout_ticks = milliseconds_to_ticks(timeout_ms);
  LightMutexHandle_t mutex = handle->common.freertos_mutex;

  if (xLightMutexLockRecursive(mutex, timeout_ticks) == pdTRUE) {
    LOG_LOCKED(handle->common.lr, myLR);
    return (true);
  }

  return (false);
}

bool mutex_lock_recursive_with_timeout(PebbleRecursiveMutex * handle, uint32_t timeout_ms) {
  uintptr_t myLR = (uintptr_t) __builtin_return_address(0);

  OS_ASSERT(!mcu_state_is_isr());

  TickType_t timeout_ticks = milliseconds_to_ticks(timeout_ms);
  LightMutexHandle_t mutex = handle->common.freertos_mutex;

  if (xLightMutexLockRecursive(mutex, timeout_ticks) == pdTRUE) {
    LOG_LOCKED(handle->common.lr, myLR);
    return (true);
  }

  return (false);
}

bool mutex_is_owned_recursive(PebbleRecursiveMutex * handle) {
  LightMutexHandle_t mutex = handle->common.freertos_mutex;
  void* holder = xLightMutexGetHolder(mutex);
  void* current = xTaskGetCurrentTaskHandle();
  return (holder == current);
}

void mutex_unlock_recursive(PebbleRecursiveMutex * handle) {
  OS_ASSERT(!mcu_state_is_isr());
  LightMutexHandle_t mutex = handle->common.freertos_mutex;
  LOG_UNLOCKED(handle->common.lr, uxLightMutexGetRecursiveCallCount(mutex));
  xLightMutexUnlockRecursive(mutex);
}
