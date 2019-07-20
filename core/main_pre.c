#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>


#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#else
    #define EMSCRIPTEN_KEEPALIVE
#endif

#include "ffi/ffi.c"

#include "core/repl_include.c"

#include "core/ringbuf_o.c"
#include "core/ringbuf_b.c"


RBB_T( out_rbb, 4096);


EMSCRIPTEN_KEEPALIVE int strCmp(const char *s1, const char *s2) {
    while (1)
    {
        int res = ((*s1 == 0) || (*s1 != *s2));
        if  (__builtin_expect((res),0))
        {
            break;
        }
        ++s1;
        ++s2;
    }
    return (*s1 - *s2);
}



// https://github.com/micropython/micropython/commit/9d8347a9aac40f8cc168b0226c2e74f776a7d4bf
// vstr_t *repl_line;
extern char *repl_line;

static int KPANIC = 0;

static int g_argc;
static char **g_argv; //[];

int endswith(const char * str, const char * suffix)
{
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return
    (str_len >= suffix_len) && (0 == strcmp(str + (str_len-suffix_len), suffix));
}


void
py_init(int argc, char *argv[]) {
    //fprintf(stdout,"create/Py_InitializeEx(0)\r\n");
    puts("create/Py_InitializeEx(0)\r\n");

    puts("Py_InitializeEx:Begin");
    Py_InitializeEx(0);
    puts("Py_InitializeEx:End");

    for (int i=0; i<argc; i++) {
        fprintf(stderr,"Micropython-wasm argv[%d]='%s'\r\n",i,argv[i]);

        if (i>1)  // skip silly "./this.program" and sys.executable
            mp_obj_list_append(mp_sys_argv, MP_OBJ_NEW_QSTR(qstr_from_str(argv[i])));
    }
}

static int SHOW_OS_LOOP=0;

EMSCRIPTEN_KEEPALIVE int show_os_loop(int state) {

    if (state>=0) {
        SHOW_OS_LOOP = state;
        if (state>0)
            fprintf(stderr,"------------- showing os loop --------------\n");
    }
    return (SHOW_OS_LOOP>0);
}

void py_boot(){

    #if WASM_FILE_API
    writecode(
        "boot.py",
        "import sys\n"
        "if not 'dev' in sys.argv:print(sys.implementation.name,'%s.%s.%s' % sys.implementation.version, sys.version, sys.platform)\n"
        "sys.path.clear()\n"
        "sys.path.append( '' )\n"
        "sys.path.append( 'assets' )\n"
        "import asyncio\n"
        "import imp\n"
    );

    PyRun_VerySimpleFile("/boot.py");
    #else
    PyRun_SimpleString("import site\n");
    #endif
}



int
py_iter_init_dlopen() {
        // dlopen not ready

        if (repl_started == -95) {
            puts("at -3 (dynamic link pass)");
        }

        // linking done, it's init time

        if (repl_started == -90) {
            puts("at -2 (interpreter init)");
            py_init(g_argc, g_argv);
        }

        if (repl_started == -80 ) {
            puts("at -1 (site.py)");
            //chdir("/data/data/u.root.upy");
            chdir("/");

            py_boot();
        }
        #if DLOPEN
        repl_started++;

        #else
        repl_started = 0;
        #endif
        return (repl_started<=0);
}

int
py_iter_init() {
        // dlopen not ready
        puts("at -3 (dynamic link pass)");
        puts("at -2 (interpreter init)");
        py_init(g_argc, g_argv);

        puts("at -1 (site.py)");
        chdir("/");
        py_boot();
        repl_started = 0;
        return (repl_started<=0);
}
