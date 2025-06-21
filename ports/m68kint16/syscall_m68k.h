// glibc, lgpl, etc

#define __NR_exit 1
#define __NR_read 3
#define __NR_write 4
#define __NR_ioctl 54
#define __NR_exit_group 94

#define INTERNAL_SYSCALL_NCS(name, err, nr, args ...)    \
    ({ unsigned long _sys_result;                         \
       {                                                  \
           /* Load argument values in temporary variables
              to perform side effects like function calls
              before the call used registers are set.  */\
           LOAD_ARGS_##nr(args)                            \
           LOAD_REGS_##nr                                   \
           register unsigned long _d0 asm ("%d0") = name;   \
           asm volatile ("trap #0"                          \
    : "=d" (_d0)                       \
    : "0" (_d0)ASM_ARGS_##nr          \
    : "memory");                       \
           _sys_result = _d0;                               \
       }                                                  \
       (long)_sys_result; })
#define INTERNAL_SYSCALL(name, err, nr, args ...)        \
    INTERNAL_SYSCALL_NCS(__NR_##name, err, nr,##args)

#undef INTERNAL_SYSCALL_ERROR_P
#define INTERNAL_SYSCALL_ERROR_P(val, err)              \
    ((unsigned long)(val) >= -4095UL)

#undef INTERNAL_SYSCALL_ERRNO
#define INTERNAL_SYSCALL_ERRNO(val, err)        (-(val))

#define LOAD_ARGS_0()
#define LOAD_REGS_0
#define ASM_ARGS_0
#define LOAD_ARGS_1(a1)                         \
    LOAD_ARGS_0()                                \
    long __arg1 = (long)(a1);
#define LOAD_REGS_1                             \
    register long _d1 asm ("d1") = __arg1;        \
    LOAD_REGS_0
#define ASM_ARGS_1      ASM_ARGS_0, "d" (_d1)
#define LOAD_ARGS_2(a1, a2)                     \
    LOAD_ARGS_1(a1)                              \
    long __arg2 = (long)(a2);
#define LOAD_REGS_2                             \
    register long _d2 asm ("d2") = __arg2;        \
    LOAD_REGS_1
#define ASM_ARGS_2      ASM_ARGS_1, "d" (_d2)
#define LOAD_ARGS_3(a1, a2, a3)                 \
    LOAD_ARGS_2(a1, a2)                          \
    long __arg3 = (long)(a3);
#define LOAD_REGS_3                             \
    register long _d3 asm ("d3") = __arg3;        \
    LOAD_REGS_2
#define ASM_ARGS_3      ASM_ARGS_2, "d" (_d3)
#define LOAD_ARGS_4(a1, a2, a3, a4)             \
    LOAD_ARGS_3(a1, a2, a3)                      \
    long __arg4 = (long)(a4);
#define LOAD_REGS_4                             \
    register long _d4 asm ("d4") = __arg4;        \
    LOAD_REGS_3
#define ASM_ARGS_4      ASM_ARGS_3, "d" (_d4)
#define LOAD_ARGS_5(a1, a2, a3, a4, a5)         \
    LOAD_ARGS_4(a1, a2, a3, a4)                  \
    long __arg5 = (long)(a5);
#define LOAD_REGS_5                             \
    register long _d5 asm ("d5") = __arg5;        \
    LOAD_REGS_4
#define ASM_ARGS_5      ASM_ARGS_4, "d" (_d5)
#define LOAD_ARGS_6(a1, a2, a3, a4, a5, a6)     \
    LOAD_ARGS_5(a1, a2, a3, a4, a5)              \
    long __arg6 = (long)(a6);
#define LOAD_REGS_6                             \
    register long _a0 asm ("a0") = __arg6;        \
    LOAD_REGS_5
#define ASM_ARGS_6      ASM_ARGS_5, "a" (_a0)
