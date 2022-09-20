#ifndef MICROPY_INCLUDED_NEUTONML_NEUTON_H
#define MICROPY_INCLUDED_NEUTONML_NEUTON_H

#include "py/obj.h"

typedef struct
{
    mp_obj_base_t base;
    bool deinited;
    uint8_t state;
    uint64_t previous_time;
    uint64_t interval;
    uint64_t pin;

} neutonml_neuton_obj_t;

#endif // MICROPY_INCLUDED_NEUTONML_NEUTON_H
