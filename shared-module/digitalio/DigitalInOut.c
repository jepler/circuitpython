#include "shared-bindings/AbstractPin.h"
#include "shared-bindings/digitalio/DigitalInOut.h"
#include "shared-module/digitalio/DigitalInOut.h"

digitalinout_result_t common_hal_digitalio_digitalinout_construct(digitalio_digitalinout_obj_t *self, mp_obj_t pin) {
    if (!abstract_pin_get_free(pin)) {
        return DIGITALINOUT_PIN_BUSY;
    }
    self->pin = pin;
    abstract_pin_claim(pin);
    // switch to input if possible, ignore any error
    (void)abstract_pin_switch_to_input(pin, PULL_NONE);
    return DIGITALINOUT_OK;
}

void common_hal_digitalio_digitalinout_deinit(digitalio_digitalinout_obj_t *self) {
    abstract_pin_reset(self->pin);
    self->pin = NULL;
}

bool common_hal_digitalio_digitalinout_deinited(digitalio_digitalinout_obj_t *self) {
    return !self->pin;
}
digitalinout_result_t common_hal_digitalio_digitalinout_switch_to_input(digitalio_digitalinout_obj_t *self, digitalio_pull_t pull) {
    return abstract_pin_switch_to_input(self, pull);
}

digitalinout_result_t common_hal_digitalio_digitalinout_switch_to_output(digitalio_digitalinout_obj_t *self, bool value, digitalio_drive_mode_t drive_mode) {
    return abstract_pin_switch_to_output(self, value, drive_mode);
}

digitalio_direction_t common_hal_digitalio_digitalinout_get_direction(digitalio_digitalinout_obj_t *self) {
    return abstract_pin_get_direction(self);
}
void common_hal_digitalio_digitalinout_set_value(digitalio_digitalinout_obj_t *self, bool value) {
    abstract_pin_set_value(self, value);
}

bool common_hal_digitalio_digitalinout_get_value(digitalio_digitalinout_obj_t *self) {
    return abstract_pin_get_value(self);
}

digitalinout_result_t common_hal_digitalio_digitalinout_set_drive_mode(digitalio_digitalinout_obj_t *self, digitalio_drive_mode_t drive_mode) {
    return abstract_pin_switch_to_output(self, common_hal_digitalio_digitalinout_get_value(self), drive_mode);
}

digitalio_drive_mode_t common_hal_digitalio_digitalinout_get_drive_mode(digitalio_digitalinout_obj_t *self) {
    return abstract_pin_get_drive_mode(self);
}
digitalinout_result_t common_hal_digitalio_digitalinout_set_pull(digitalio_digitalinout_obj_t *self, digitalio_pull_t pull) {
    return abstract_pin_switch_to_input(self, pull);
}
digitalio_pull_t common_hal_digitalio_digitalinout_get_pull(digitalio_digitalinout_obj_t *self) {
    return abstract_pin_get_pull(self);
}
void common_hal_digitalio_digitalinout_never_reset(digitalio_digitalinout_obj_t *self) {
    abstract_pin_never_reset(self);
}
