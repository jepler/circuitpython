#pragma once

#include "py/obj.h"
#include "py/proto.h"

typedef enum _digitalio_direction_t {
    DIRECTION_INPUT,
    DIRECTION_OUTPUT
} digitalio_direction_t;

typedef enum _digitalio_pull_t {
    PULL_NONE,
    PULL_UP,
    PULL_DOWN
} digitalio_pull_t;

typedef enum {
    DRIVE_MODE_PUSH_PULL,
    DRIVE_MODE_OPEN_DRAIN
} digitalio_drive_mode_t;

typedef enum {
    DIGITALINOUT_OK,
    DIGITALINOUT_PIN_BUSY,
    #if CIRCUITPY_DIGITALIO_HAVE_INPUT_ONLY
    DIGITALINOUT_INPUT_ONLY,
    #endif
    #if CIRCUITPY_DIGITALIO_HAVE_INVALID_PULL
    DIGITALINOUT_INVALID_PULL,
    #endif
    #if CIRCUITPY_DIGITALIO_HAVE_INVALID_DRIVE_MODE
    DIGITALINOUT_INVALID_DRIVE_MODE,
    #endif
} digitalinout_result_t;

typedef struct _circuitpython_pin_p_t {
    MP_PROTOCOL_HEAD // MP_QSTR_Pin
    digitalinout_result_t (*switch_to_input)(mp_obj_t self_in, digitalio_pull_t pull);
    digitalinout_result_t (*switch_to_output)(mp_obj_t self_in, digitalio_drive_mode_t drive_mode);
    void (*set_value)(mp_obj_t self_in, bool value);

    bool (*get_value)(mp_obj_t self_in);
    qstr (*get_name)(mp_obj_t self_in);
    digitalio_pull_t (*get_pull)(mp_obj_t self_in);
    digitalio_drive_mode_t (*get_drive_mode)(mp_obj_t self_in);
    digitalio_direction_t (*get_direction)(mp_obj_t obj);

    bool (*get_free)(mp_obj_t self_in);
    void (*claim)(mp_obj_t self_in);
    void (*never_reset)(mp_obj_t self_in);
    void (*reset)(mp_obj_t self_in);
} circuitpython_pin_p_t;

bool abstract_pin_check(mp_obj_t obj);
void assert_abstract_pin_free(mp_obj_t obj);

mp_obj_t mp_arg_validate_type_abstract_pin(mp_obj_t obj, qstr arg_name);
mp_obj_t mp_arg_validate_free_abstract_pin(mp_obj_t obj, qstr arg_name);
mp_obj_t mp_arg_validate_free_abstract_pin_or_none(mp_obj_t obj, qstr arg_name);
size_t mp_arg_validate_list_is_free_abstract_pins(qstr arg_name, mp_obj_t pins_in, mp_obj_t **pins_out);

// These assume the object was previously checked with abstract_pin_check or mp_arg_validate_type_abstract_pin
digitalinout_result_t abstract_pin_switch_to_input(mp_obj_t obj, digitalio_pull_t pull);
digitalinout_result_t abstract_pin_switch_to_output(mp_obj_t obj, bool value, digitalio_drive_mode_t drive_mode);
void abstract_pin_set_value(mp_obj_t obj, bool value);
bool abstract_pin_get_value(mp_obj_t obj);
digitalio_pull_t abstract_pin_get_pull(mp_obj_t obj);
digitalio_drive_mode_t abstract_pin_get_drive_mode(mp_obj_t obj);
digitalio_direction_t abstract_pin_get_direction(mp_obj_t obj);
qstr abstract_pin_get_name(mp_obj_t obj);
bool abstract_pin_get_free(mp_obj_t obj);
void abstract_pin_claim(mp_obj_t obj);
void abstract_pin_never_reset(mp_obj_t obj);
void abstract_pin_reset(mp_obj_t obj);
