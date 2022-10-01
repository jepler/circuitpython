#include "py/runtime.h"

#include "shared-module/neutonml/Neuton.h"
#include "shared-bindings/neutonml/Neuton.h"

void shared_module_neutonml_neuton_construct(neutonml_neuton_obj_t *self) {
    self->deinited = 0;
    self->state = 0x00;
}

bool shared_module_neutonml_neuton_deinited(neutonml_neuton_obj_t *self) {
    return self->deinited;
}

void shared_module_neutonml_neuton_deinit(neutonml_neuton_obj_t *self) {
    self->deinited = 1;
}

///
/// \brief Get element count of array that you should pass to neuton_model_set_inputs() function
/// \return Array elements count
///
uint16_t shared_module_neutonml_neuton_model_inputs_count(neutonml_neuton_obj_t *self) {
    return neuton_model_inputs_count();
}

///
/// \brief Set input values
/// \param inputs - input_t[] array of neuton_model_inputs_count() elements
/// \return Zero if model ready for prediction. Result < 0 indicates error, result > 0 - model not ready for prediction.
///
int8_t shared_module_neutonml_neuton_model_set_inputs(
    neutonml_neuton_obj_t *self, input_t *inputs) {
    return neuton_model_set_inputs(inputs);
}

///
/// \brief Set ready flag
///
void shared_module_neutonml_neuton_model_set_ready_flag(neutonml_neuton_obj_t *self) {
    return neuton_model_set_ready_flag();
}

///
/// \brief Get model inputs array
/// \return Pointer to model inputs
///
input_t *shared_module_neutonml_neuton_model_get_inputs_ptr(neutonml_neuton_obj_t *self) {
    return neuton_model_get_inputs_ptr();
}

///
/// \brief Reset input values
///
void shared_module_neutonml_neuton_model_reset_inputs(neutonml_neuton_obj_t *self) {
    return neuton_model_reset_inputs();
}

///
/// \brief Get element count of array that neuton_model_run_inference() returns
/// \return Array elements count
///
uint16_t shared_module_neutonml_neuton_model_outputs_count(neutonml_neuton_obj_t *self) {
    return neuton_model_outputs_count();
}

///
/// \brief Make a prediction
/// \param index - pointer to predicted class variable (binary/multi classification). Can be NULL.
/// \param outputs - float[] array of neuton_model_outputs_count() elements, contains predicted target variable
///                  (for regression task) or probabilities of each class (binary/multi classification).
/// \return Zero on successful prediction. Result > 0 - model not ready for prediction.
///
int8_t shared_module_neutonml_neuton_model_run_inference(neutonml_neuton_obj_t *self, uint16_t *index, float **outputs) {
    return neuton_model_run_inference(index, outputs);
}

///
/// \brief Get task type
/// \return Task type value
///
TaskType shared_module_neutonml_neuton_model_task_type(neutonml_neuton_obj_t *self) {
    return neuton_model_task_type();
}

///
/// \brief Get model quantization level
/// \return Quantization level (possible values: 8, 16, 32)
///
uint8_t shared_module_neutonml_neuton_model_quantization_level(neutonml_neuton_obj_t *self) {
    return neuton_model_quantization_level();
}

///
/// \brief Get float support flag
/// \return Flag value (possible values: 0, 1)
///
uint8_t shared_module_neutonml_neuton_model_float_calculations(neutonml_neuton_obj_t *self) {
    return neuton_model_float_calculations();
}

///
/// \brief Get model neurons count
/// \return Neurons count
///
uint16_t shared_module_neutonml_neuton_model_neurons_count(neutonml_neuton_obj_t *self) {
    return neuton_model_neurons_count();
}

///
/// \brief Get model weights count
/// \return Weights count
///
uint32_t shared_module_neutonml_neuton_model_weights_count(neutonml_neuton_obj_t *self) {
    return neuton_model_weights_count();
}

///
/// \brief Get element count of input normalization array
/// \return Array elements count
///
uint16_t shared_module_neutonml_neuton_model_inputs_limits_count(neutonml_neuton_obj_t *self) {
    return neuton_model_inputs_limits_count();
}

///
/// \brief Get window size
/// \return Window size
///
uint16_t shared_module_neutonml_neuton_model_window_size(neutonml_neuton_obj_t *self) {
    return neuton_model_window_size();
}

///
/// \brief Get model RAM usage
/// \return RAM usage in bytes
///
uint32_t shared_module_neutonml_neuton_model_ram_usage(neutonml_neuton_obj_t *self) {
    return neuton_model_ram_usage();
}

///
/// \brief Get model size
/// \return Model size without meta information
///
uint32_t shared_module_neutonml_neuton_model_size(neutonml_neuton_obj_t *self) {
    return neuton_model_size();
}

///
/// \brief Get model & meta information size
/// \return Model size with meta information
///
uint32_t shared_module_neutonml_neuton_model_size_with_meta(neutonml_neuton_obj_t *self) {
    return neuton_model_size_with_meta();
}
