#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_NEUTONML_NEUTON_H
#define MICROPY_INCLUDED_SHARED_BINDINGS_NEUTONML_NEUTON_H

typedef struct
{
    mp_obj_base_t base;
    bool deinited;
    uint8_t state;
} neutonml_neuton_obj_t;

extern const mp_obj_type_t neutonml_neuton_type;
#endif // MICROPY_INCLUDED_SHARED_BINDINGS_NEUTONML_NEUTON_H
