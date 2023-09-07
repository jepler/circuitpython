#include "shared-bindings/AbstractPin.h"
#include "py/runtime.h"

bool abstract_pin_check(mp_obj_t obj) {
    return mp_proto_get(MP_QSTR_Pin, obj) != NULL;
}

void mp_arg_validate_type_abstract_pin(mp_obj_t obj, qstr arg_name) {
    if (!abstract_pin_check(obj)) {
        mp_raise_TypeError_varg(translate("%q must be of type %q, not %q"), arg_name, MP_QSTR_Pin, mp_obj_get_type(obj)->name);
    }
}


// Note: this multiply evaluates the "pin" expression, so don't be fancy!
// ** Assumes that `pin` was not previously verified to implement the protocol **
#define _abstract_pin_method(pin, meth, ...) (((circuitpython_pin_p_t *)mp_proto_get(MP_QSTR_Pin, (pin)))->meth((pin),##__VA_ARGS__))

digitalinout_result_t abstract_pin_switch_to_input(mp_obj_t obj, digitalio_pull_t pull) {
    return _abstract_pin_method(obj, switch_to_input, pull);
}

digitalinout_result_t abstract_pin_switch_to_output(mp_obj_t obj, bool value, digitalio_drive_mode_t drive_mode) {
    digitalinout_result_t r = _abstract_pin_method(obj, switch_to_output, drive_mode);
    if (r == DIGITALINOUT_OK) {
        _abstract_pin_method(obj, set_value, value);
    }
    return r;
}

void abstract_pin_set_value(mp_obj_t obj, bool value) {
    return _abstract_pin_method(obj, set_value, value);
}

bool abstract_pin_get_value(mp_obj_t obj) {
    return _abstract_pin_method(obj, get_value);
}

digitalio_pull_t abstract_pin_get_pull(mp_obj_t obj) {
    return _abstract_pin_method(obj, get_pull);
}

digitalio_drive_mode_t abstract_pin_get_drive_mode(mp_obj_t obj) {
    return _abstract_pin_method(obj, get_drive_mode);
}

bool abstract_pin_get_free(mp_obj_t obj) {
    return _abstract_pin_method(obj, get_free);
}

void abstract_pin_claim(mp_obj_t obj) {
    return _abstract_pin_method(obj, claim);
}

void abstract_pin_never_reset(mp_obj_t obj) {
    return _abstract_pin_method(obj, never_reset);
}
