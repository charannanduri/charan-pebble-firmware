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

#include "util/attributes.h"
#include "util/likely.h"

#include <stdint.h>

NORETURN os_assertion_failed(const char *filename, int line);
NORETURN os_assertion_failed_lr(const char *filename, int line, uint32_t lr);

#ifdef PLATFORM_ESP32S3
// ESP32-S3 doesn't have __FILE_NAME__, use __FILE__ instead
#define OS_ASSERT(expr) \
  do { \
    if (UNLIKELY(!(expr))) { \
      os_assertion_failed(__FILE__, __LINE__); \
    } \
  } while (0)

#define OS_ASSERT_LR(expr, lr) \
  do { \
    if (UNLIKELY(!(expr))) { \
      os_assertion_failed_lr(__FILE__, __LINE__, lr); \
    } \
  } while (0)
#else
#define OS_ASSERT(expr) \
  do { \
    if (UNLIKELY(!(expr))) { \
      os_assertion_failed(__FILE_NAME__, __LINE__); \
    } \
  } while (0)

#define OS_ASSERT_LR(expr, lr) \
  do { \
    if (UNLIKELY(!(expr))) { \
      os_assertion_failed_lr(__FILE_NAME__, __LINE__, lr); \
    } \
  } while (0)
#endif
