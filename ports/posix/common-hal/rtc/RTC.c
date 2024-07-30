/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Jeff Epler
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

#include "shared-bindings/rtc/RTC.h"
#include "py/runtime.h"
#include <errno.h>
#include <sys/time.h>
#include <time.h>

extern void common_hal_rtc_get_time(timeutils_struct_time_t *tm) {
    struct tm tu;
    struct timeval ti;
    gettimeofday(&ti, NULL);
    localtime_r(&ti.tv_sec, &tu);
    tm->tm_year = tu.tm_year + 1900;
    tm->tm_mon = tu.tm_mon;
    tm->tm_mday = tu.tm_mday;
    tm->tm_hour = tu.tm_hour;
    tm->tm_min = tu.tm_min;
    tm->tm_sec = tu.tm_sec;
    tm->tm_wday = tu.tm_wday;
    tm->tm_yday = tu.tm_yday;
}

extern void common_hal_rtc_set_time(timeutils_struct_time_t *tm) {
    struct timeval tu = { .tv_sec = TIMEUTILS_SECONDS_1970_TO_2000 + timeutils_seconds_since_2000(tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec) };
    int r = settimeofday(&tu, NULL);
    if (r < 0) {
        mp_raise_OSError(errno);
    }
}

extern int common_hal_rtc_get_calibration(void) {
    return 0;
}
extern void common_hal_rtc_set_calibration(int calibration) {
    mp_arg_validate_int_range(calibration, 0, 0, MP_QSTR_calibration);
}
