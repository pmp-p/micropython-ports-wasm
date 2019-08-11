/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include "py/mpconfig.h"


#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "py/runtime.h"
#include "py/smallint.h"
#include "py/mphal.h"
#include "extmod/utime_mphal.h"

#include <stdio.h>


#include <time.h>
#include <sys/time.h>

// mingw32 defines CLOCKS_PER_SEC as ((clock_t)<somevalue>) but preprocessor does not handle casts
#if defined(__MINGW32__) && !defined(__MINGW64_VERSION_MAJOR)
#define MP_REMOVE_BRACKETSA(x)
#define MP_REMOVE_BRACKETSB(x) MP_REMOVE_BRACKETSA x
#define MP_REMOVE_BRACKETSC(x) MP_REMOVE_BRACKETSB x
#define MP_CLOCKS_PER_SEC MP_REMOVE_BRACKETSC(CLOCKS_PER_SEC)
#else
#define MP_CLOCKS_PER_SEC CLOCKS_PER_SEC
#endif

#if defined(MP_CLOCKS_PER_SEC)
#define CLOCK_DIV (MP_CLOCKS_PER_SEC / 1000.0F)
#else
#error Unsupported clock() implementation
#endif


#define show_clock 1

#if show_clock

#if 1
    // Note: this is deprecated since CPy3.3, but pystone still uses it.
    STATIC mp_obj_t mod_time_clock(void) {
    #if MICROPY_PY_BUILTINS_FLOAT
        // float cannot represent full range of int32 precisely, so we pre-divide
        // int to reduce resolution, and then actually do float division hoping
        // to preserve integer part resolution.
        return mp_obj_new_float((float)(clock() / 1000) / CLOCK_DIV);
    #else
        return mp_obj_new_int((mp_int_t)clock());
    #endif
    }
#else
    STATIC mp_obj_t mod_time_clock(void) {

            return 0;
    }
#endif

STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_time_clock_obj, mod_time_clock);
#endif



STATIC mp_obj_t mod_time_time(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

#if MICROPY_PY_BUILTINS_FLOAT
    mp_float_t val = tv.tv_sec + (mp_float_t)tv.tv_usec / 1000000;
    return mp_obj_new_float(val);
#else
    return mp_obj_new_int_from_uint( tv.tv_sec );
#endif
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_time_time_obj, mod_time_time);

STATIC mp_obj_t mod_time_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return mp_obj_new_int_from_uint( ts.tv_nsec );
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_time_time_ns_obj, mod_time_time_ns);

STATIC
mp_obj_t mod_time_sleep(mp_obj_t arg) {
    //NO SYNC SLEEPon wasm, use asyncio
    printf("mod_time_sleep");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_time_sleep_obj, mod_time_sleep);


STATIC
mp_obj_t mod_time_sleep_ms(mp_obj_t arg) {
    //NO SYNC SLEEPon wasm, use asyncio
    printf("mod_time_sleep_ms");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_time_sleep_ms_obj, mod_time_sleep_ms);

STATIC
mp_obj_t mod_time_sleep_us(mp_obj_t arg) {
    //NO SYNC SLEEPon wasm, use asyncio
    printf("mod_time_sleep_us");
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(mod_time_sleep_us_obj, mod_time_sleep_us);



STATIC mp_obj_t mod_time_localtime(size_t n_args, const mp_obj_t *args) {
    time_t t;
    if (n_args == 0) {
        t = time(NULL);
    } else {
        #if MICROPY_PY_BUILTINS_FLOAT
        mp_float_t val = mp_obj_get_float(args[0]);
        t = (time_t)MICROPY_FLOAT_C_FUN(trunc)(val);
        #else
        t = mp_obj_get_int(args[0]);
        #endif
    }
    struct tm *tm = localtime(&t);

    mp_obj_t ret = mp_obj_new_tuple(9, NULL);

    mp_obj_tuple_t *tuple = MP_OBJ_TO_PTR(ret);
    tuple->items[0] = MP_OBJ_NEW_SMALL_INT(tm->tm_year + 1900);
    tuple->items[1] = MP_OBJ_NEW_SMALL_INT(tm->tm_mon + 1);
    tuple->items[2] = MP_OBJ_NEW_SMALL_INT(tm->tm_mday);
    tuple->items[3] = MP_OBJ_NEW_SMALL_INT(tm->tm_hour);
    tuple->items[4] = MP_OBJ_NEW_SMALL_INT(tm->tm_min);
    tuple->items[5] = MP_OBJ_NEW_SMALL_INT(tm->tm_sec);
    int wday = tm->tm_wday - 1;
    if (wday < 0) {
        wday = 6;
    }
    tuple->items[6] = MP_OBJ_NEW_SMALL_INT(wday);
    tuple->items[7] = MP_OBJ_NEW_SMALL_INT(tm->tm_yday + 1);
    tuple->items[8] = MP_OBJ_NEW_SMALL_INT(tm->tm_isdst);

    return ret;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_time_localtime_obj, 0, 1, mod_time_localtime);

STATIC const mp_rom_map_elem_t mp_module_time_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_utime) },
#if  show_clock
	{ MP_ROM_QSTR(MP_QSTR_clock), MP_ROM_PTR(&mod_time_clock_obj) },
#endif
    { MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&mod_time_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_ms), MP_ROM_PTR(&mod_time_sleep_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep_us), MP_ROM_PTR(&mod_time_sleep_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&mod_time_time_obj) },
    { MP_ROM_QSTR(MP_QSTR_time_ns), MP_ROM_PTR(&mod_time_time_ns_obj) },
    { MP_ROM_QSTR(MP_QSTR_localtime), MP_ROM_PTR(&mod_time_localtime_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_ms), MP_ROM_PTR(&mp_utime_ticks_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_us), MP_ROM_PTR(&mp_utime_ticks_us_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_cpu), MP_ROM_PTR(&mp_utime_ticks_cpu_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_add), MP_ROM_PTR(&mp_utime_ticks_add_obj) },
    { MP_ROM_QSTR(MP_QSTR_ticks_diff), MP_ROM_PTR(&mp_utime_ticks_diff_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_time_globals, mp_module_time_globals_table);

const mp_obj_module_t mp_module_time = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_time_globals,
};

