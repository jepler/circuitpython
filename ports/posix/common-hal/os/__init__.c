#include "shared-bindings/os/__init__.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include <fcntl.h>
#include <sys/utsname.h>
#include <unistd.h>

static int random_fd = -1;
bool common_hal_os_urandom(uint8_t *buffer, mp_uint_t length) {
    if (random_fd == -1) {
        random_fd = open("/dev/urandom", O_RDONLY);
    }
    ssize_t result = read(random_fd, buffer, length);
    return result != -1 && result == (ssize_t)length;
}

STATIC const MP_DEFINE_STR_OBJ(os_uname_info_sysname_obj, "posix");
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_release_obj, MICROPY_VERSION_STRING);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_version_obj, MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_machine_obj, MICROPY_HW_BOARD_NAME " with " MICROPY_HW_MCU_NAME);

STATIC const qstr os_uname_info_fields[] = {
    MP_QSTR_sysname, MP_QSTR_nodename,
    MP_QSTR_release, MP_QSTR_version, MP_QSTR_machine
};

mp_obj_t common_hal_os_uname(void) {
    struct utsname buf;
    uname(&buf);
    mp_obj_t items[5] = {
        (mp_obj_t)&os_uname_info_sysname_obj,
        mp_obj_new_str(buf.nodename, strlen(buf.nodename)),
        (mp_obj_t)&os_uname_info_release_obj,
        (mp_obj_t)&os_uname_info_version_obj,
        (mp_obj_t)&os_uname_info_machine_obj
    };
    return mp_obj_new_attrtuple(os_uname_info_fields, 5, items);
}
