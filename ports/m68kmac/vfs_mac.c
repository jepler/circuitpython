#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/stream.h"
#include "extmod/vfs.h"

#include "macutil.h"
#include "vfs_mac.h"
#include "debug_print.h"

#define MAC_O_RDONLY (1)
#define MAC_O_WRONLY (2)
#define MAC_O_RDWR (3)
#define MAC_O_TRUNC (4)
#define MAC_O_APPEND (8)
#define MAC_O_CREAT (16)

typedef struct _mp_obj_vfs_mac_t {
    mp_obj_base_t base;
    INTEGER volRefNum;
    size_t root_len;
    bool readonly;
} mp_obj_vfs_mac_t;

typedef struct _mp_obj_vfs_mac_file_t {
    mp_obj_base_t base;
    INTEGER volRefNum;
    INTEGER fd;
    int mode;
} mp_obj_vfs_mac_file_t;

static VCB *getVolumeByName(mp_obj_t name) {
    GET_STR_DATA_LEN(name, str_data, str_len);
    VCB *vol = (VCB *)LMGetVCBQHdr().qHead;
    while (vol) {
        if (PSTR_LEN(vol->vcbVN) == str_len &&
            memcmp(PSTR_DATA(vol->vcbVRefNum), str_data, str_len) == 0) {
            return vol;
        }
        vol = (VCB *)vol->qLink;
    }
    mp_raise_ValueError(MP_ERROR_TEXT("volume not found"));
}

static VCB *getVolumeByVolumeReference(INTEGER vn) {
    VCB *vol = (VCB *)LMGetVCBQHdr().qHead;
    while (vol) {
        if (vol->vcbVRefNum == vn) {
            return vol;
        }
        vol = (VCB *)vol->qLink;
    }
    mp_raise_ValueError(MP_ERROR_TEXT("volume not found"));
}


static void check_fd_is_open(const mp_obj_vfs_mac_file_t *o) {
    if (o->fd < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("I/O operation on closed file"));
    }
}

static void vfs_mac_file_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    mp_obj_vfs_mac_file_t *self = MP_OBJ_TO_PTR(self_in);
    VCB *vol = getVolumeByVolumeReference(self->volRefNum);
    mp_printf(print, "<io.%s %d on %.*s>",
        mp_obj_get_type_str(self_in), self->fd,
        (int)PSTR_LEN(vol->vcbVN), PSTR_DATA(vol->vcbVN));
}

mp_obj_t mp_vfs_mac_file_open(const mp_obj_vfs_mac_t *fs, const mp_obj_type_t *type, mp_obj_t file_in, mp_obj_t mode_in) {
    const char *mode_s = mp_obj_str_get_str(mode_in);

    int mode_rw = 0, mode_x = 0;
    while (*mode_s) {
        switch (*mode_s++) {
            case 'r':
                mode_rw = MAC_O_RDONLY;
                break;
            case 'w':
                mode_rw = MAC_O_WRONLY;
                mode_x = MAC_O_CREAT | MAC_O_TRUNC;
                break;
            case 'a':
                mode_rw = MAC_O_WRONLY;
                mode_x = MAC_O_CREAT | MAC_O_APPEND;
                break;
            case '+':
                mode_rw = MAC_O_RDWR;
                break;
            case 'b':
                type = &mp_type_vfs_mac_fileio;
                break;
            case 't':
                type = &mp_type_vfs_mac_textio;
                break;
        }
    }

    mp_obj_vfs_mac_file_t *o = mp_obj_malloc_with_finaliser(mp_obj_vfs_mac_file_t, type);
    o->fd = -1; // In case open() fails below, initialise this as a "closed" file object.

    mp_obj_t fid = file_in;

    if (mp_obj_is_small_int(fid)) {
        o->fd = MP_OBJ_SMALL_INT_VALUE(fid);
        return MP_OBJ_FROM_PTR(o);
    }

    Str255 pName;
    PSTR_FROM_STR(pName, fid);
    INTEGER fd;
    OSErr err = noErr;

    while (true) {
        OSErr err = FSOpen(pName, fs->volRefNum, &fd);
        DPRINTF("FSOpen(..., %d) -> %d\n", fs->volRefNum, err);
        if (err == fnfErr && (mode_x & MAC_O_CREAT)) {
            err = Create(pName, fs->volRefNum, 'mupy', type == &mp_type_vfs_mac_textio ? 'text' : 'bin ');
            DPRINTF("Create() -> %d\n", err);
            if (err != noErr) {
                raise_mac_err(err);
            }
            mode_x &= ~MAC_O_CREAT;
            continue;
        }
        if (err != noErr) {
            raise_mac_err(err);
        }
        break;
    }

    if (mode_x & MAC_O_TRUNC) {
        err = SetEOF(fd, 0);
    } else if (mode_x & MAC_O_APPEND) {
        err = SetFPos(fd, fsFromLEOF, 0);
    }
    if (err != noErr) {
        FSClose(fd);
        raise_mac_err(err);
    }
    o->fd = fd;
    o->mode = mode_rw;
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t vfs_mac_file_fileno(mp_obj_t self_in) {
    mp_obj_vfs_mac_file_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(self->fd);
}
static MP_DEFINE_CONST_FUN_OBJ_1(vfs_mac_file_fileno_obj, vfs_mac_file_fileno);

static mp_uint_t vfs_mac_file_read(mp_obj_t o_in, void *buf, mp_uint_t size, int *errcode) {
    mp_obj_vfs_mac_file_t *o = MP_OBJ_TO_PTR(o_in);

    check_fd_is_open(o);
    long count = (long)size;
    OSErr err = FSRead(o->fd, &count, buf);

    if (err == noErr) {
        return count;
    }
    *errcode = convert_mac_err(err);
    return MP_STREAM_ERROR;
}

static mp_uint_t vfs_mac_file_write(mp_obj_t o_in, const void *buf, mp_uint_t size, int *errcode) {
    mp_obj_vfs_mac_file_t *o = MP_OBJ_TO_PTR(o_in);

    check_fd_is_open(o);
    long count = (long)size;
    OSErr err = FSWrite(o->fd, &count, buf);

    if (err == noErr) {
        return count;
    }
    *errcode = convert_mac_err(err);
    return MP_STREAM_ERROR;
}
static mp_uint_t vfs_mac_file_ioctl(mp_obj_t o_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    mp_obj_vfs_mac_file_t *o = MP_OBJ_TO_PTR(o_in);

    if (request != MP_STREAM_CLOSE) {
        check_fd_is_open(o);
    }

    switch (request) {
        case MP_STREAM_FLUSH: {
            INTEGER err = FlushVol(NULL, o->volRefNum);
            if (err == noErr) {
                *errcode = convert_mac_err(err);
                break;
            }
            return 0;
        }
        case MP_STREAM_SEEK: {
            struct mp_stream_seek_t *s = (struct mp_stream_seek_t *)arg;
            INTEGER posMode =
                s->whence == MP_SEEK_CUR ? fsFromMark :
                s->whence == MP_SEEK_SET ? fsFromStart : fsFromMark;
            OSErr err = SetFPos(o->fd, posMode, (long)s->offset);
            // TODO: seek that enlarges a file [need to call Allocate()?]
            if (err == noErr) {
                *errcode = convert_mac_err(err);
                break;
            }
            return 0;
        }
        case MP_STREAM_CLOSE:
            if (o->fd >= 0) {
                int fd = o->fd;
                o->fd = -1;
                MP_THREAD_GIL_EXIT();
                mp_printf(&mp_plat_print, "closing %d\n", fd);
                OSErr err = FSClose(fd);
                OSErr err1 = FlushVol(NULL, o->volRefNum);
                if (err != noErr) {
                    *errcode = convert_mac_err(err);
                    break;
                }
                if (err1 != noErr) {
                    *errcode = convert_mac_err(err1);
                    break;
                }
                mp_printf(&mp_plat_print, "closed %d\n", fd);
                MP_THREAD_GIL_ENTER();
            }
            return 0;
        case MP_STREAM_GET_FILENO:
            return o->fd;

        default:
            *errcode = EINVAL;
    }
    return MP_STREAM_ERROR;
}

static const mp_rom_map_elem_t vfs_mac_rawfile_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_fileno), MP_ROM_PTR(&vfs_mac_file_fileno_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&mp_stream___exit___obj) },
};

static MP_DEFINE_CONST_DICT(vfs_mac_rawfile_locals_dict, vfs_mac_rawfile_locals_dict_table);

static const mp_stream_p_t vfs_mac_fileio_stream_p = {
    .read = vfs_mac_file_read,
    .write = vfs_mac_file_write,
    .ioctl = vfs_mac_file_ioctl,
};

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_vfs_mac_fileio,
    MP_QSTR_FileIO,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    print, vfs_mac_file_print,
    protocol, &vfs_mac_fileio_stream_p,
    locals_dict, &vfs_mac_rawfile_locals_dict
    );

static const mp_stream_p_t vfs_mac_textio_stream_p = {
    .read = vfs_mac_file_read,
    .write = vfs_mac_file_write,
    .ioctl = vfs_mac_file_ioctl,
    .is_text = true,
};

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_vfs_mac_textio,
    MP_QSTR_TextIOWrapper,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    print, &vfs_mac_file_print,
    protocol, &vfs_mac_textio_stream_p,
    locals_dict, &vfs_mac_rawfile_locals_dict
    );

static mp_obj_t vfs_mac_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    INTEGER volRefNum;

    if (n_args == 0) {
        GetVol(NULL, &volRefNum);
    } else {
        volRefNum = getVolumeByName(args[0])->vcbVRefNum;
    }

    // create new object
    mp_obj_vfs_mac_t *vfs = mp_obj_malloc(mp_obj_vfs_mac_t, type);
    vfs->volRefNum = volRefNum;

    return MP_OBJ_FROM_PTR(vfs);
}

static mp_obj_t volumes(mp_obj_t self_in) {
    mp_obj_t result = mp_obj_list_make_new(&mp_type_list, 0, 0, NULL);
    VCB *vol = (VCB *)LMGetVCBQHdr().qHead;
    mp_obj_t args[3];
    mp_load_method(result, MP_QSTR_append, args);
    while (vol) {
        args[2] = new_str_from_pstr(vol->vcbVN);
        mp_call_method_n_kw(1, 0, args);
        vol = (VCB *)vol->qLink;
    }
    return result;
}
MP_DEFINE_CONST_FUN_OBJ_1(volumes_obj, volumes);

static mp_obj_t vfs_mac_mount(mp_obj_t self_in, mp_obj_t readonly, mp_obj_t mkfs) {
    mp_obj_vfs_mac_t *self = MP_OBJ_TO_PTR(self_in);
    if (mp_obj_is_true(readonly)) {
        self->readonly = true;
    }
    if (mp_obj_is_true(mkfs)) {
        mp_raise_OSError(MP_EPERM);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(vfs_mac_mount_obj, vfs_mac_mount);

static mp_obj_t vfs_mac_umount(mp_obj_t self_in) {
    (void)self_in;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(vfs_mac_umount_obj, vfs_mac_umount);

static mp_obj_t vfs_mac_open(mp_obj_t self_in, mp_obj_t path_in, mp_obj_t mode_in) {
    mp_obj_vfs_mac_t *self = MP_OBJ_TO_PTR(self_in);
    const char *mode = mp_obj_str_get_str(mode_in);
    if (self->readonly
        && (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL || strchr(mode, '+') != NULL)) {
        mp_raise_OSError(MP_EROFS);
    }

    return mp_vfs_mac_file_open(self, &mp_type_vfs_mac_textio, path_in, mode_in);
}
static MP_DEFINE_CONST_FUN_OBJ_3(vfs_mac_open_obj, vfs_mac_open);


static const mp_rom_map_elem_t vfs_mac_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_volumes), MP_ROM_PTR(&volumes_obj) },
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&vfs_mac_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&vfs_mac_umount_obj) },
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&vfs_mac_open_obj) },

    #if 0
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

static void vfs_mac_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_obj_vfs_mac_t *self = MP_OBJ_TO_PTR(self_in);
    VCB *vol = getVolumeByVolumeReference(self->volRefNum);
    mp_printf(print, "<VfsMac %d %.*s>",
        self->volRefNum, (int)PSTR_LEN(vol->vcbVN), PSTR_DATA(vol->vcbVN));
}

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_vfs_mac,
    MP_QSTR_VfsMac,
    MP_TYPE_FLAG_NONE,
    make_new, vfs_mac_make_new,
    protocol, &vfs_mac_proto,
    locals_dict, &vfs_mac_locals_dict,
    print, &vfs_mac_print
    );
