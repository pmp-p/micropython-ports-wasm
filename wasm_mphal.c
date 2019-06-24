#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "extmod/misc.h"

#include <stdarg.h>

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#elif __CPP__
    #define EMSCRIPTEN_KEEPALIVE
#endif


#include "upython.h"


#include <time.h>

// use nanosec timer from emscripten_now_res

mp_uint_t mp_hal_ticks_ms(void) {
    struct timespec ts;
    clock_getres(CLOCK_MONOTONIC, &ts);
    return ts.tv_nsec * 1000000;
}

mp_uint_t mp_hal_ticks_us(void) {
    struct timespec ts;
    clock_getres(CLOCK_MONOTONIC, &ts);
    return ts.tv_nsec * 1000;
}


// Receive single character
int mp_hal_stdin_rx_chr(void) {

    fprintf(stderr,"mp_hal_stdin_rx_chr");
    unsigned char c = fgetc(stdin);
    return c;
}



//FIXME: libc print with valid json are likely to pass and get interpreted by pts
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    for(int i=0;i<len;i++)
        printf("{\"%c\":%u}\n",49,(unsigned char)str[i]);
}


EMSCRIPTEN_KEEPALIVE static PyObject *
embed_run_script(PyObject *self, PyObject *argv) {
    char *runstr = NULL;
    if (!PyArg_ParseTuple(argv, "s", &runstr)) {
        return NULL;
    }
    emscripten_run_script( runstr );
    Py_RETURN_NONE;
}


#if MICROPY_USE_READLINE == 0
char *prompt(char *p) {
    fprintf(stderr,"61:simple read string\n");
    static char buf[256];
    //fputs(p, stdout);
    fputs(p, stderr);
    char *s = fgets(buf, sizeof(buf), stdin);
    if (!s) {
        return NULL;
    }
    int l = strlen(buf);
    if (buf[l - 1] == '\n') {
        buf[l - 1] = 0;
    } else {
        l++;
    }
    char *line = malloc(l);
    memcpy(line, buf, l);
    return line;
}
#endif
