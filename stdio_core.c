#include <unistd.h>
#include "stdio.h"
#include "py/mpconfig.h"

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = fgetc(stdin);
    return c;
}

char fb[80*25];

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    if (len==1)
        printf("%c%c%c\n",str[0],16,3);
    else
    //fwrite(str, len, 1, stdout);
/*    snprintf(&fb[0],len,"%s",str); //,16,3);
    fb[len+1]=0;
    printf("%s%c%c\n",str,16,3); */
        printf("%s %d\n",str,len);
}
