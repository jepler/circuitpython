#ifndef MICROPY_INCLUDED_SHARED_BINDINGS_NEUTONML_NEUTON_H
#define MICROPY_INCLUDED_SHARED_BINDINGS_NEUTONML_NEUTON_H

#include "shared-module/neutonml/Neuton.h"

extern const mp_obj_type_t neutonml_neuton_type;

extern void shared_module_neutonml_neuton_construct(neutonml_neuton_obj_t *self);
extern void shared_module_neutonml_neuton_deinit(neutonml_neuton_obj_t *self);
extern bool shared_module_neutonml_neuton_deinited(neutonml_neuton_obj_t *self);
extern const char *shared_module_neutonml_neuton_get_question(neutonml_neuton_obj_t *self);
extern mp_int_t shared_module_neutonml_neuton_get_answer(neutonml_neuton_obj_t *self);

#endif // MICROPY_INCLUDED_SHARED_BINDINGS_NEUTONML_NEUTON_H
