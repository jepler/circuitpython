#include <stdint.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "shared-bindings/neutonml/__init__.h"
#include "shared-bindings/neutonml/Neuton.h"

STATIC const mp_rom_map_elem_t neutonml_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_neutonml)},
    {MP_ROM_QSTR(MP_QSTR_neutonml), MP_ROM_PTR(&neutonml_neuton_type)},
};

STATIC MP_DEFINE_CONST_DICT(neutonml_module_globals, neutonml_module_globals_table);

const mp_obj_module_t neutonml_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&neutonml_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_neutonml, neutonml_module, AULITECH_NEUTONML);
