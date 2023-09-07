#pragma once
#include "py/obj.h"

typedef struct {
    mp_obj_base_t base;
    mp_obj_t pin;
} digitalio_digitalinout_obj_t;
