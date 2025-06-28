#include <py/runtime.h>
#include "macutil.h"

mp_obj_t new_str_from_pstr(Byte *pStr) {
    return mp_obj_new_str((const char *)pStr + 1, *pStr);
}
