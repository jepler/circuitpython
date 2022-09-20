#include "py/runtime.h"

#include "shared-bindings/neutonml/__init__.h"
#include "shared-bindings/neutonml/Neuton.h"
#include "Neuton.h"

void shared_module_neutonml_neuton_construct(neutonml_neuton_obj_t *self, mp_int_t pin, mp_int_t interval) {
    self->deinited = 0;
    self->state = 0x00;
    self->interval = interval;
    self->pin = pin;
}

bool shared_module_neutonml_neuton_deinited(neutonml_neuton_obj_t *self) {
    return self->deinited;
}

void shared_module_neutonml_neuton_deinit(neutonml_neuton_obj_t *self) {
    self->deinited = 1;
}

const char *shared_module_neutonml_neuton_get_question(neutonml_neuton_obj_t *self) {
    return "Tricky...";
}

mp_int_t shared_module_neutonml_neuton_get_answer(neutonml_neuton_obj_t *self) {
    return self->pin;
}
