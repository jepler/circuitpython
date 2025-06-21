#include <stddef.h>
#include <sys/types.h>
#include <errno.h>
#include "syscall_m68k.h"

ssize_t write(int fd, void *buf, size_t cnt) {
    long sys_result = INTERNAL_SYSCALL(write, err, 3, fd, buf, cnt);
    if (INTERNAL_SYSCALL_ERROR_P(sys_result, err)) {
        errno = INTERNAL_SYSCALL_ERRNO(sys_result, err);
        return -1;
    }
    return sys_result;
}

ssize_t read(int fd, void *buf, size_t cnt) {
    long sys_result = INTERNAL_SYSCALL(read, err, 3, fd, buf, cnt);
    if (INTERNAL_SYSCALL_ERROR_P(sys_result, err)) {
        errno = INTERNAL_SYSCALL_ERRNO(sys_result, err);
        return -1;
    }
    return sys_result;
}


void exit(int status) {
    INTERNAL_SYSCALL(exit, err, 1, status);
    __builtin_trap();
}
