#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "upython.c"



#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#else
    #define EMSCRIPTEN_KEEPALIVE
#endif

#include "ffi/ffi.c"



// https://github.com/micropython/micropython/commit/9d8347a9aac40f8cc168b0226c2e74f776a7d4bf
// vstr_t *repl_line;
extern char *repl_line;

static int repl_started = -1;
static int KPANIC = 0;

size_t
bsd_strlen(const char *str) {
        const char *s;
        for (s = str; *s; ++s);
        return (s - str);
}

static int SHOW_OS_LOOP=0;

EMSCRIPTEN_KEEPALIVE int
show_os_loop(int state) {
    int last = SHOW_OS_LOOP;
    if (state>=0) {
        SHOW_OS_LOOP = state;
        if (state>0) {
            //fprintf(stderr,"------------- showing os loop --------------\n");
            fprintf(stderr,"------------- showing os loop / starting repl --------------\n");
            repl_started = 1;
        } else {
            if (last!=state)
                fprintf(stderr,"------------- hiding os loop --------------\n");
        }
    }
    return (last>0);
}


void
py_iter_one(void){

    // shm not ready
    if (repl_started<0) return;

    // io demux will be done here too via io_loop(json_state)
    if (!KPANIC)
       if (repl_line[0]){
            //fprintf(stderr,"IO %lu [%s]\n", strlen(repl_line), repl_line);
            if (!do_code(repl_line, IS_STR)) {
                fprintf(stderr,"kernel panic %lu [%s]\n", strlen(repl_line), repl_line);
                KPANIC = 1;
            }
            repl_line[0]=0;
        }


    // call asyncio auto stepping first in case no repl
    if (!KPANIC)
        PyRun_SimpleString("asyncio.step()");

    // running repl after script in cpython is sys.flags.inspect, should monitor and init repl

    // repl not ready
    if (!repl_started) return;

    // should give a way here to discard repl events feeding  "await input()" instead

    while (1) {
        int rx = EM_ASM_INT({
            if (window.stdin_array.length)
                return window.stdin_array.shift();
            return 0;
        });

        if (rx) {
            if (rx==12) {
                //clear screen
                fprintf(stderr,"IO(12): Form Feed ");
                PyRun_SimpleString("#\n");
            }

            if (rx>127)
                fprintf(stderr, "FIXME:stdin-utf8:%u\n", rx );
            pyexec_event_repl_process_char(rx);
        } else break;
    }

}

/* =====================================================================================
    bad sync experiment with file access trying to help on
        https://github.com/littlevgl/lvgl/issues/792

    status: better than nothing.
*/

#include "wasm_file_api.c"
#include "wasm_import_api.c"

//=====================================================================================



#if WASM_FILE_API
EMSCRIPTEN_KEEPALIVE void
repl(const char *code) {
    int stl = strlen(code);
    if (stl>REPL_INPUT_MAX){
        stl=REPL_INPUT_MAX;
        fprintf( stderr, "REPL Buffer overflow: %i > %i", stl, REPL_INPUT_MAX);
    }
    strncpy(repl_line, code, stl);
    repl_line[stl]=0;
}


EMSCRIPTEN_KEEPALIVE void
writecode(char *filename,char *code) {
    EM_ASM({
        FS.createDataFile("/", UTF8ToString($0), UTF8ToString($1) , true, true);
    }, filename, code);
}

#endif

#if 1

int
await_dlopen(const char *def){
    return !EM_ASM_INT( { return defined(UTF8ToString($0), window.lib); }, def );
}

// should check null

EMSCRIPTEN_KEEPALIVE char*
repl_init() {
    if (repl_started<0) {
        repl_line = (char *)malloc(REPL_INPUT_SIZE);
        // this is for null end str !!!!!
        for (int i=0;i<REPL_INPUT_SIZE;i++)
            repl_line[i] = 0;
        repl_started = 0;
    }
    return &repl_line[0];
}

EMSCRIPTEN_KEEPALIVE int
repl_run(int warmup) {
    if (!warmup) {
        pyexec_event_repl_init();
        repl_started = REPL_INPUT_MAX;
    }
    return REPL_INPUT_MAX;
}





int
main(int argc, char *argv[]) {

#if 0
    //keep symbol global for wasm debugging
    fprintf(stderr,"//#FIXME: add sys.executable to sys\n");

    void *lib_handle = dlopen("lib/libtest.wasm", RTLD_NOW | RTLD_GLOBAL);
    if (!lib_handle) {
        puts("cannot load side module");
    } else
        puts("+1 browser can dlopen side wasm library");


    lib_handle = dlopen("libmicropython.wasm", RTLD_NOW | RTLD_GLOBAL);
    if (!lib_handle) {
        puts("-1 browser can't dlopen main dynamically linking error or wasm 4KB limitation, using static instead");
    } else
        puts("+1 browser can dlopen main wasm library");

    // NOT USING dlopen() and libmicropython.wasm ATM because chrome will fail
    // and that would mean compiling to shared library as .js
/*
    Error in loading dynamic library libmicropython.wasm:
    RangeError: WebAssembly.Compile is disallowed on the main thread, if the buffer size is larger than 4KB.
    Use WebAssembly.compile, or compile on a worker thread.
*/
#else
    #pragma message (" ----------- dlopen off for upstream, no FFI ! ---------")
#endif

    //setenv("HOME","/data/data/u.root.upy",0);
    setenv("HOME","/",1);

    //setenv("MICROPYPATH","/data/data/u.root.upy/assets",0);
    setenv("MICROPYPATH","/",1);

    fprintf(stdout,"create/Py_InitializeEx(0)\r\n");
    Py_InitializeEx(0);

    for (int i=0; i<argc; i++) {
        fprintf(stderr,"Micropython-wasm argv[%d]='%s'\r\n",i,argv[i]);
        if (i>1)  // skip silly "./this.program" and sys.executable
            mp_obj_list_append(mp_sys_argv, MP_OBJ_NEW_QSTR(qstr_from_str(argv[i])));
    }

    //chdir("/data/data/u.root.upy");
    chdir("/");

    fprintf(stderr," =================================================\r\n");
    PyArg_ParseTuple(nullptr,"%s\n","argv1");
    fprintf(stderr," =================================================\r\n");

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
    PyRun_SimpleString("import boot\n");
    #endif


    emscripten_set_main_loop( py_iter_one, 0, 1);  // <= this will exit to js now.

}

#else


















#endif
