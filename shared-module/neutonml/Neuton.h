#ifndef MICROPY_INCLUDED_NEUTONML_NEUTON_H
#define MICROPY_INCLUDED_NEUTONML_NEUTON_H

#include "py/obj.h"
#include "shared-bindings/neutonml/Neuton.h"
#include "neuton.h"

extern void shared_module_neutonml_neuton_construct(neutonml_neuton_obj_t *self);
extern void shared_module_neutonml_neuton_deinit(neutonml_neuton_obj_t *self);
extern bool shared_module_neutonml_neuton_deinited(neutonml_neuton_obj_t *self);

extern uint16_t shared_module_neutonml_neuton_model_inputs_count(neutonml_neuton_obj_t *self);
extern int8_t shared_module_neutonml_neuton_model_set_inputs(neutonml_neuton_obj_t *self, input_t *inputs);
extern void shared_module_neutonml_neuton_model_set_ready_flag(neutonml_neuton_obj_t *self);
extern input_t *shared_module_neutonml_neuton_model_get_inputs_ptr(neutonml_neuton_obj_t *self);
extern void shared_module_neutonml_neuton_model_reset_inputs(neutonml_neuton_obj_t *self);
extern uint16_t shared_module_neutonml_neuton_model_outputs_count(neutonml_neuton_obj_t *self);
extern int8_t shared_module_neutonml_neuton_model_run_inference(neutonml_neuton_obj_t *self, uint16_t *index, float **outputs);

extern TaskType shared_module_neutonml_neuton_model_task_type(neutonml_neuton_obj_t *self);
extern uint8_t shared_module_neutonml_neuton_model_quantization_level(neutonml_neuton_obj_t *self);
extern uint8_t shared_module_neutonml_neuton_model_float_calculations(neutonml_neuton_obj_t *self);
extern uint16_t shared_module_neutonml_neuton_model_neurons_count(neutonml_neuton_obj_t *self);
extern uint32_t shared_module_neutonml_neuton_model_weights_count(neutonml_neuton_obj_t *self);
extern uint16_t shared_module_neutonml_neuton_model_inputs_limits_count(neutonml_neuton_obj_t *self);
extern uint16_t shared_module_neutonml_neuton_model_window_size(neutonml_neuton_obj_t *self);
extern uint32_t shared_module_neutonml_neuton_model_ram_usage(neutonml_neuton_obj_t *self);
extern uint32_t shared_module_neutonml_neuton_model_size(neutonml_neuton_obj_t *self);
extern uint32_t shared_module_neutonml_neuton_model_size_with_meta(neutonml_neuton_obj_t *self);

#endif // MICROPY_INCLUDED_NEUTONML_NEUTON_H
