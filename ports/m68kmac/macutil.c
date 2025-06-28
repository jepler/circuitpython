#include <py/runtime.h>
#include <py/objstr.h>
#include <py/mperrno.h>
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


int convert_mac_err(OSErr e) {
    int err = MP_EINVAL;
    switch (e) {
        case nsvErr: // "No such volume"
        case noMacDskErr:
        case extFSErr:
        case badMDBErr:
            err = MP_ENODEV;
            break;
        case gfpErr: // "Error during GetFPos"
        case fsRnErr: // "Problem during rename"
        case posErr:
        case bdNamErr:
        case paramErr:
        case rfNumErr:
            err = MP_EINVAL;
            break;
        case dirFulErr:
            err = MP_ENFILE;
            break;
        case dskFulErr:
            err = MP_EPIPE;
            break;
        case dupFNErr:
            err = MP_EEXIST;
            break;
        case ioErr:
        case eofErr:
            err = MP_EIO;
            break;
        case fBsyErr:
            err = MP_EBUSY;
            break;
        case volOffLinErr:
            err = MP_EISCONN;
            break;
        case fLckdErr:
            err = MP_EWOULDBLOCK;
            break;
        case dirNFErr:
        case fnfErr:
            err = MP_ENOENT;
            break;
        case fnOpnErr:
            err = MP_EBADF;
            break;
        case wPrErr:
        case wrPermErr:
        case vLckdErr:
        case permErr:
        case opWrErr:
            err = MP_EPERM;
            break;
        case tmwdoErr:
        case tmfoErr:
            err = MP_EMFILE;
            break;
        case wrgVolTypErr:
            err = MP_EINVAL;
            break;
    }
    return err;
}
MP_NORETURN void raise_mac_err(OSErr e) {
    mp_raise_OSError(convert_mac_err(e));
}
