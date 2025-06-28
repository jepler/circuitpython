#include <py/runtime.h>
#include <py/objstr.h>
#include "macutil.h"

mp_obj_t new_str_from_pstr(Byte *pStr) {
    return mp_obj_new_str((const char *)pStr + 1, *pStr);
}

Byte *pstr_from_str(Byte *pStr, size_t pStr_size, mp_obj_t obj) {
    GET_STR_DATA_LEN(obj, str_data, str_len);
    if (str_len + 1 >= pStr_size) {
        mp_raise_ValueError(MP_ERROR_TEXT("buffer too long"));
    }
    *pStr = str_len;
    memcpy(pStr + 1, str_data, str_len);
    return pStr;
}
