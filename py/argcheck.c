/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * SPDX-FileCopyrightText: Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "py/runtime.h"

#include "supervisor/shared/translate.h"

void mp_arg_check_num_sig(size_t n_args, size_t n_kw, uint32_t sig) {
    // TODO maybe take the function name as an argument so we can print nicer error messages

    // The reverse of MP_OBJ_FUN_MAKE_SIG
    bool takes_kw = sig & 1;
    size_t n_args_min = sig >> 17;
    size_t n_args_max = (sig >> 1) & 0xffff;

    if (n_kw && !takes_kw) {
        #if MICROPY_ERROR_REPORTING <= MICROPY_ERROR_REPORTING_TERSE
        mp_arg_error_terse_mismatch();
        #else
        mp_raise_TypeError(MP_ERROR_TEXT("function doesn't take keyword arguments"));
        #endif
    }

    if (n_args_min == n_args_max) {
        if (n_args != n_args_min) {
            #if MICROPY_ERROR_REPORTING <= MICROPY_ERROR_REPORTING_TERSE
            mp_arg_error_terse_mismatch();
            #else
            mp_raise_TypeError_varg(MP_ERROR_TEXT("function takes %d positional arguments but %d were given"),
                n_args_min, n_args);
            #endif
        }
    } else {
        if (n_args < n_args_min) {
            #if MICROPY_ERROR_REPORTING <= MICROPY_ERROR_REPORTING_TERSE
            mp_arg_error_terse_mismatch();
            #else
            mp_raise_TypeError_varg(
                MP_ERROR_TEXT("function missing %d required positional arguments"),
                n_args_min - n_args);
            #endif
        } else if (n_args > n_args_max) {
            #if MICROPY_ERROR_REPORTING <= MICROPY_ERROR_REPORTING_TERSE
            mp_arg_error_terse_mismatch();
            #else
            mp_raise_TypeError_varg(
                MP_ERROR_TEXT("function expected at most %d arguments, got %d"),
                n_args_max, n_args);
            #endif
        }
    }
}

inline void mp_arg_check_num(size_t n_args, mp_map_t *kw_args, size_t n_args_min, size_t n_args_max, bool takes_kw) {
    size_t n_kw = 0;
    if (kw_args != NULL) {
        n_kw = kw_args->used;
    }
    mp_arg_check_num_sig(n_args, n_kw, MP_OBJ_FUN_MAKE_SIG(n_args_min, n_args_max, takes_kw));
}

inline void mp_arg_check_num_kw_array(size_t n_args, size_t n_kw, size_t n_args_min, size_t n_args_max, bool takes_kw) {
    mp_arg_check_num_sig(n_args, n_kw, MP_OBJ_FUN_MAKE_SIG(n_args_min, n_args_max, takes_kw));
}

static mp_arg_val_t get_defval(const mp_arg_t *arginfo) {
    mp_arg_val_t result = arginfo->defval;

    int kind = arginfo->flags & MP_ARG_KIND_MASK;
    if (kind >= MP_ARG_FUNC) {
        if (arginfo->flags & MP_ARG_FLOAT) {
            if (arginfo->flags & MP_ARG_OR_NONE) {
                result.u_float = MICROPY_FLOAT_C_FUN(nan)("");
            } else {
                result.u_float = -1;
            }
        } else if (kind >= MP_ARG_RANGE16) {
            result.u_int = -1;
        } else {
            result.u_obj = mp_const_none;
        }
    }

    return result;
}

static mp_arg_val_t validate_convert_argument(const mp_arg_t *arginfo, mp_obj_t given_arg) {
    mp_arg_val_t result = {.u_obj = given_arg };
    uint16_t arg_name = arginfo->qst;

    int kind = arginfo->flags & MP_ARG_KIND_MASK;

    if (arginfo->flags & MP_ARG_OR_MINUS1) {
        if ((mp_obj_is_int(given_arg) && mp_obj_get_int(given_arg) == -1)
            || ((kind & MP_ARG_FLOAT) && mp_obj_is_float(given_arg) && mp_obj_get_float(given_arg) == MICROPY_FLOAT_CONST(-1.))) {
            if (kind & MP_ARG_FLOAT) {
                result.u_float = MICROPY_FLOAT_CONST(-1.);
            }
            if (kind >= MP_ARG_NUMBER) {
                result.u_int = -1;
            }
            return result;
        }
    }

    if (arginfo->flags & MP_ARG_OR_NONE) {
        if (given_arg == mp_const_none) {
            if (kind & MP_ARG_FLOAT) {
                result.u_float = MICROPY_FLOAT_C_FUN(nan)("");
            }
            if (kind >= MP_ARG_NUMBER) {
                result.u_int = -1;
            }
            return result;
        }
    }

    int lo = 0, hi;

    switch (kind) {

        case MP_ARG_BOOL:
            result.u_bool = mp_obj_is_true(given_arg);
            return result;

        case MP_ARG_OBJ:
            return result;

        case MP_ARG_TYPE:
            mp_arg_validate_type(given_arg, arginfo->defval.u_obj, arg_name);
            return result;

        case MP_ARG_XTYPE:
            if (!mp_obj_is_subclass_fast(mp_obj_get_type(given_arg), arginfo->defval.u_obj)) {
                mp_obj_type_t *type = MP_OBJ_TO_PTR(arginfo->defval.u_obj);
                mp_raise_TypeError_varg(translate("%q must of type %q or a subclass"), arg_name, type->name);
            }
            return result;

        case MP_ARG_FUNC:
            return arginfo->defval.u_func(arginfo, given_arg);

        case MP_ARG_NUMBER:
            if (arginfo->flags & MP_ARG_FLOAT) {
                result.u_float = mp_obj_get_float(given_arg);
            } else {
                result.u_int = mp_obj_get_int(given_arg);
            }
            return result;

        case MP_ARG_RANGE16:
            lo = arginfo->defval.u_slo;
            hi = arginfo->defval.u_shi;
            break;

        case MP_ARG_URANGE16:
            lo = arginfo->defval.u_ulo;
            hi = arginfo->defval.u_uhi;
            break;

        case MP_ARG_1_TO_N:
            lo++;
            MP_FALLTHROUGH;

        case MP_ARG_0_TO_N:
            hi = arginfo->defval.u_int;
            break;

        case MP_ARG_POW2: {
            int val = mp_obj_get_int(given_arg);
            if (val & (val - 1)) {
                mp_raise_TypeError_varg(translate("%q must be a power of 2"), arginfo->qst);
            }
            lo = 1 << arginfo->defval.u_ulo;
            hi = 1 << arginfo->defval.u_uhi;
            break;
        }

        default:
            assert(false);
    }

    const char *extra = "";
    if (arginfo->flags & MP_ARG_OR_NONE) {
        if (arginfo->flags & MP_ARG_OR_MINUS1) {
            extra = ", None, or -1";
        } else {
            extra = " or None";
        }
    } else if (arginfo->flags & MP_ARG_OR_MINUS1) {
        extra = " or -1";
    }

    if (arginfo->flags & MP_ARG_FLOAT) {
        result.u_float = mp_obj_get_float(given_arg);
        if (result.u_float < lo || result.u_float > hi) {
            mp_raise_ValueError_varg(translate("%q must be %d-%d%s"), arg_name, lo, hi, extra);
        }
        return result;
    }

    result.u_int = mp_obj_get_int(given_arg);
    if (result.u_int < lo || result.u_int > hi) {
        mp_raise_ValueError_varg(translate("%q must be %d-%d%s"), arg_name, lo, hi, extra);
    }
    return result;
}

void mp_arg_parse_all(size_t n_pos, const mp_obj_t *pos, mp_map_t *kws, size_t n_allowed, const mp_arg_t *allowed, mp_arg_val_t *out_vals) {
    size_t pos_found = 0, kws_found = 0;
    for (size_t i = 0; i < n_allowed; i++) {
        mp_obj_t given_arg;
        if (i < n_pos) {
            if (allowed[i].flags & MP_ARG_KW_ONLY) {
                goto extra_positional;
            }
            pos_found++;
            given_arg = pos[i];
        } else {
            mp_map_elem_t *kw = NULL;
            if (kws != NULL) {
                kw = mp_map_lookup(kws, MP_OBJ_NEW_QSTR(allowed[i].qst), MP_MAP_LOOKUP);
            }
            if (kw == NULL) {
                if (allowed[i].flags & MP_ARG_REQUIRED) {
                    #if MICROPY_ERROR_REPORTING <= MICROPY_ERROR_REPORTING_TERSE
                    mp_arg_error_terse_mismatch();
                    #else
                    mp_raise_TypeError_varg(MP_ERROR_TEXT("'%q' argument required"), allowed[i].qst);
                    #endif
                }
                out_vals[i] = get_defval(&allowed[i]);
                continue;
            } else {
                kws_found++;
                given_arg = kw->value;
            }
        }
        if (allowed[i].flags & MP_ARG_SEQUENCE_OF) {
            mp_int_t len = MP_OBJ_SMALL_INT_VALUE(mp_obj_len(given_arg));
            for (mp_int_t j = 0; j < len; j++) {
                (void)validate_convert_argument(&allowed[i], mp_obj_subscr(given_arg, MP_OBJ_NEW_SMALL_INT(i), MP_OBJ_SENTINEL));
            }
            out_vals[i].u_obj = given_arg;
        } else {
            out_vals[i] = validate_convert_argument(&allowed[i], given_arg);
        }
    }

    if (pos_found < n_pos) {
    extra_positional:
        #if MICROPY_ERROR_REPORTING <= MICROPY_ERROR_REPORTING_TERSE
        mp_arg_error_terse_mismatch();
        #else
        // TODO better error message
        mp_raise_TypeError(MP_ERROR_TEXT("extra positional arguments given"));
        #endif
    }
    if (kws != NULL && kws_found < kws->used) {
        #if MICROPY_ERROR_REPORTING <= MICROPY_ERROR_REPORTING_TERSE
        mp_arg_error_terse_mismatch();
        #else
        // TODO better error message
        mp_raise_TypeError(MP_ERROR_TEXT("extra keyword arguments given"));
        #endif
    }
}

void mp_arg_parse_all_kw_array(size_t n_pos, size_t n_kw, const mp_obj_t *args, size_t n_allowed, const mp_arg_t *allowed, mp_arg_val_t *out_vals) {
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_pos);
    mp_arg_parse_all(n_pos, args, &kw_args, n_allowed, allowed, out_vals);
}

NORETURN void mp_arg_error_terse_mismatch(void) {
    mp_raise_TypeError(MP_ERROR_TEXT("argument num/types mismatch"));
}

#if MICROPY_CPYTHON_COMPAT
NORETURN void mp_arg_error_unimpl_kw(void) {
    mp_raise_NotImplementedError(MP_ERROR_TEXT("keyword argument(s) not yet implemented - use normal args instead"));
}
#endif


mp_int_t mp_arg_validate_int_min(mp_int_t i, mp_int_t min, qstr arg_name) {
    if (i < min) {
        mp_raise_ValueError_varg(translate("%q must be >= %d"), arg_name, min);
    }
    return i;
}

mp_int_t mp_arg_validate_int_max(mp_int_t i, mp_int_t max, qstr arg_name) {
    if (i > max) {
        mp_raise_ValueError_varg(translate("%q must <= %d"), arg_name, max);
    }
    return i;
}

mp_int_t mp_arg_validate_int_range(mp_int_t i, mp_int_t min, mp_int_t max, qstr arg_name) {
    if (i < min || i > max) {
        mp_raise_ValueError_varg(translate("%q must be %d-%d"), arg_name, min, max);
    }
    return i;
}

mp_float_t mp_arg_validate_obj_float_non_negative(mp_obj_t float_in, mp_float_t default_for_null, qstr arg_name) {
    const mp_float_t f = (float_in == MP_OBJ_NULL)
        ? default_for_null
        : mp_obj_get_float(float_in);
    if (f <= (mp_float_t)0.0) {
        mp_raise_ValueError_varg(translate("%q must be >= 0"), arg_name);
    }
    return f;
}

size_t mp_arg_validate_length_with_name(mp_int_t i, size_t length, qstr arg_name, qstr length_name) {
    if (i != (mp_int_t)length) {
        mp_raise_ValueError_varg(translate("%q length must be %q"), MP_QSTR_pressed, MP_QSTR_num_keys);
    }
    return (size_t)i;
}

mp_obj_t mp_arg_validate_type(mp_obj_t obj, const mp_obj_type_t *type, qstr arg_name) {
    if (!mp_obj_is_type(obj, type)) {
        mp_raise_TypeError_varg(translate("%q must of type %q"), arg_name, type->name);
    }
    return obj;
}

mp_obj_t mp_arg_validate_string(mp_obj_t obj, qstr arg_name) {
    if (!mp_obj_is_str(obj)) {
        mp_raise_TypeError_varg(translate("%q must be a string"), arg_name);
    }
    return obj;
}
