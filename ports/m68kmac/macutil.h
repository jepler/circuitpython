#include <py/obj.h>
#include "Multiverse.h"

MP_NORETURN void raise_mac_err(OSErr e);
int convert_mac_err(OSErr e);
mp_obj_t new_str_from_pstr(Byte *pStr);
Byte *pstr_from_str(Byte *pStr, size_t pStr_size, mp_obj_t obj);

#define PSTR_FROM_STR(pStr, obj) pstr_from_str(pStr, MP_ARRAY_SIZE(pStr), obj)

#define PSTR_LEN(p) (*(p))
#define PSTR_DATA(p) ((char *)((p) + 1))
