#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "extmod/vfs.h"

#include "macutil.h"

typedef struct _mp_obj_vfs_mac_t {
    mp_obj_base_t base;
    vstr_t root;
    size_t root_len;
    bool readonly;
} mp_obj_vfs_mac_t;

static mp_obj_t vfs_mac_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);

    // create new object
    mp_obj_vfs_mac_t *vfs = mp_obj_malloc(mp_obj_vfs_mac_t, type);

    return MP_OBJ_FROM_PTR(vfs);
}

static mp_obj_t volumes(mp_obj_t self_in) {
    mp_obj_t result = mp_obj_list_make_new(&mp_type_list, 0, 0, NULL);
    VCB *vol = (VCB *)LMGetVCBQHdr().qHead;
    mp_obj_t args[3];
    mp_load_method(result, MP_QSTR_append, args);
    while (vol) {
        args[2] = new_str_from_pstr(vol->vcbVN + 1);
        mp_call_method_n_kw(1, 0, args);
        vol = (VCB *)vol->qLink;
    }
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_1(volumes_obj, volumes);

static const mp_rom_map_elem_t vfs_mac_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_volumes), MP_ROM_PTR(&volumes_obj) },
    #if 0
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&vfs_mac_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&vfs_mac_umount_obj) },
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&vfs_mac_open_obj) },

    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&vfs_mac_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&vfs_mac_getcwd_obj) },
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&vfs_mac_ilistdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&vfs_mac_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&vfs_mac_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&vfs_mac_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&vfs_mac_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&vfs_mac_stat_obj) },
    #if MICROPY_PY_OS_STATVFS
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&vfs_mac_statvfs_obj) },
    #endif
    #endif
};
static MP_DEFINE_CONST_DICT(vfs_mac_locals_dict, vfs_mac_locals_dict_table);

static const mp_vfs_proto_t vfs_mac_proto = {
    #if 0
    .import_stat = mp_vfs_mac_import_stat,
    #endif
};

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_vfs_mac,
    MP_QSTR_VfsMac,
    MP_TYPE_FLAG_NONE,
    make_new, vfs_mac_make_new,
    protocol, &vfs_mac_proto,
    locals_dict, &vfs_mac_locals_dict
    );
