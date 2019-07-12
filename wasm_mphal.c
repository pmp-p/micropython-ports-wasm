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


struct timespec ts;
#define EPOCH_US 0
#if EPOCH_US
static unsigned long epoch_us = 0;
#endif

mp_uint_t mp_hal_ticks_ms(void) {
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long now_us = ts.tv_sec * 1000000 + ts.tv_nsec / 1000 ;
#if EPOCH_US
    if (!epoch_us)
        epoch_us = now_us -1000;
    return (mp_uint_t)( (now_us - epoch_us) / 1000 ) ;
#else
    return  (mp_uint_t)(now_us / 1000);
#endif

}

mp_uint_t mp_hal_ticks_us(void) {
    clock_gettime(CLOCK_MONOTONIC, &ts);
    unsigned long now_us = ts.tv_sec * 1000000 + ts.tv_nsec / 1000 ;
#if EPOCH_US
    if (!epoch_us)
        epoch_us = now_us-1;
    return (mp_uint_t)(now_us - epoch_us);
#else
    return  (mp_uint_t)now_us;
#endif
}




// Receive single character
int mp_hal_stdin_rx_chr(void) {

    fprintf(stderr,"mp_hal_stdin_rx_chr");
    unsigned char c = fgetc(stdin);
    return c;
}

static unsigned char last = 0;


//FIXME: libc print with valid json are likely to pass and get interpreted by pts
//TODO: buffer all until render tick
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    for(int i=0;i<len;i++) {
        if ( (str[i] == 0x0a) && (last != 0x0d) )
            printf("{\"%c\":%u}\n",49, 0x0d );
        printf("{\"%c\":%u}\n",49,(unsigned char)str[i]);
        last = (unsigned char)str[i];
    }
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


#if MICROPY_USE_READLINE

#else

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
