#pragma once
#include "supervisor/shared/tick.h"
void mp_hal_set_interrupt_char(int c);
#define mp_hal_ticks_ms()       ((mp_uint_t)supervisor_ticks_ms32())
