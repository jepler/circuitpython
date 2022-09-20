#ifndef MICROPY_INCLUDED_NEUTONML_NEUTON_H
#define MICROPY_INCLUDED_NEUTONML_NEUTON_H

#include "py/obj.h"

typedef struct
{
    mp_obj_base_t base;
    bool deinited;
} neutonml_neuton_obj_t;

#endif // MICROPY_INCLUDED_NEUTONML_NEUTON_H
