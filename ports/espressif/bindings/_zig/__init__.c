#include "py/obj.h"
#include "py/runtime.h"
#include "common-hal/_zig/__init__.h"

static mp_obj_t zig_light_get_power(void) {
    return mp_obj_new_bool(common_hal_zig_light_get_power());
}
MP_DEFINE_CONST_FUN_OBJ_0(zig_light_get_power_obj, zig_light_get_power);

static const mp_rom_map_elem_t zig_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__zig) },

    { MP_ROM_QSTR(MP_QSTR_light_get_power), MP_ROM_PTR(&zig_light_get_power_obj)},
};

static MP_DEFINE_CONST_DICT(zig_module_globals, zig_module_globals_table);

const mp_obj_module_t zig_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&zig_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR__zig, zig_module);
