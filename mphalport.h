#include <unistd.h>
//#include "py/nlr.h"
//#include "py/compile.h"
#include "py/runtime.h"
//#include "py/repl.h"
//#include "py/gc.h"
//#include "lib/utils/pyexec.h"

#ifndef CHAR_CTRL_C
#define CHAR_CTRL_C (3)
#endif


static inline void mp_hal_set_interrupt_char(char c) {}

// TODO: wasm does not sleep
/*
static inline void mp_hal_delay_ms(mp_uint_t ms) { usleep((ms) * 1000); }
static inline void mp_hal_delay_us(mp_uint_t us) { usleep(us); }
#define mp_hal_ticks_cpu() 0
*/

mp_uint_t mp_hal_ticks_ms(void);



#define RAISE_ERRNO(err_flag, error_val) \
    { if (err_flag == -1) \
        { mp_raise_OSError(error_val); } }



#define nullptr NULL
#define Py_RETURN_NONE return nullptr;
#define PyObject mp_obj_t


int PyArg_ParseTuple(PyObject *argv, const char *fmt, ...);
