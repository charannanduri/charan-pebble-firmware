#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    OS_OK = 0,
    OS_ERROR = -1,
    OS_TIMEOUT = -2,
    OS_NO_MEMORY = -3,
    OS_INVALID_PARAM = -4
} OSStatus;

typedef void* OSHandle;
typedef uint32_t OSTime;

void os_init(void);

#define OS_UNUSED(x) ((void)(x))
#define OS_ARRAY_LENGTH(arr) (sizeof(arr) / sizeof((arr)[0]))
#define OS_ALIGN(size, align) (((size) + (align) - 1) & ~((align) - 1))
