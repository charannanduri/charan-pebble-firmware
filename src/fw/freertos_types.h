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

#pragma once

#ifndef ESP_PLATFORM // Only define these if not building with ESP-IDF

// Dummy definitions of RTOS types for cases where we don't have the real ones (e.g. unit tests)

// FIXME: PBL-21049 Fix platform abstraction and board definition scheme

// This file gets included in syscall/syscall.h before FreeRTOS.h gets included
// These types are used in syscall handler definitions, so we need to define them here
// They should match the real FreeRTOS definitions
typedef void * QueueHandle_t;

typedef QueueHandle_t SemaphoreHandle_t;

typedef void * TaskHandle_t;

#endif // ESP_PLATFORM

typedef void (*TaskFunction_t)( void * );

typedef struct xTASK_PARAMETERS TaskParameters_t;

typedef struct xMEMORY_REGION MemoryRegion_t;
