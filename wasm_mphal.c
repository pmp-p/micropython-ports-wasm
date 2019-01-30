#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdio.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "extmod/misc.h"


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


// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    if (len==1)
        printf("%c%c%c\n",str[0],16,3);
    else
        printf("%s %lu\n",str,len);
}
