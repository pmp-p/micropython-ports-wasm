#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdio.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "extmod/misc.h"


#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#else
    #define EMSCRIPTEN_KEEPALIVE
#endif


mp_uint_t mp_hal_ticks_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

mp_uint_t mp_hal_ticks_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}



// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = fgetc(stdin);
    return c;
}

char unistash={0};

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    if (len==1){
        if (unistash){
            printf("%c%c%c%c\n",unistash,str[0],16,3);
        }
        if (str[0]>127){
            unistash = str[0];
            EM_ASM({
               console.log("unicode found");
            });
        }
        printf("%c%c%c\n",str[0],16,3);
        return ;
    }
    //should not happen, also buffered output is bad for terminal/repl use since emscripten will only flush on \n
    printf("%s %lu\n",str,len);
}


/*
mp_obj_t
mp_builtin_open_obj(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    printf("mp_builtin_open_obj");
    return mp_const_none;
}
*/