#pragma once

#include <stdint.h>
#include <stdbool.h>

void tick_service_init(void);
uint32_t tick_get_count(void);
uint32_t tick_to_ms(uint32_t ticks);
uint32_t ms_to_tick(uint32_t ms);

#define TICK_FREQUENCY_HZ 1000
