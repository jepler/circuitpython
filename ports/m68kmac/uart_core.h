#pragma once

#include "py/mpprint.h"

extern mp_print_t debug_print;
#define DEBUG_PRINT(...) mp_print(&debug_print, __VA_ARGS__)
