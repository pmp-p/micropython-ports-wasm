#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "upython.h"



#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#else
    #define EMSCRIPTEN_KEEPALIVE
#endif

#include "ffi/ffi.c"


// https://github.com/micropython/micropython/commit/9d8347a9aac40f8cc168b0226c2e74f776a7d4bf
// vstr_t *repl_line;
extern char *repl_line;

void
py_iter_one(void){
    /*
    int rx = mp_hal_stdin_rx_chr();
    if (rx && rx!=10)
        fprintf(stderr, "stdin:%u\n", rx );
*/
    if (repl_line[0]){
        PyRun_SimpleString(repl_line);
        repl_line[0]=0;
    }
}

EMSCRIPTEN_KEEPALIVE void
repl(const char *code) {
    int stl = strlen(code);
    if (stl>REPL_INPUT_SIZE){
        stl=REPL_INPUT_SIZE;
        fprintf( stderr, "REPL Buffer overflow: %i > %i", stl, REPL_INPUT_SIZE);
    }
    strncpy(repl_line, code, stl);
}

EMSCRIPTEN_KEEPALIVE void
writecode(char *filename,char *code) {
    EM_ASM({
    FS.createDataFile("/", Pointer_stringify($0), Pointer_stringify($1) , true, true);
    }, filename, code);
}

#if 1

int
await_dlopen(const char *def){
    return !EM_ASM_INT( { return defined(Pointer_stringify($0), window.lib); }, def );
}




int
main(int argc, char *argv[]) {

    //keep symbol global for wasm debugging
    fprintf(stderr,"//#FIXME: add sys.executable to sys\n");
    void *lib_handle = dlopen("lib/libtest.wasm", RTLD_NOW | RTLD_GLOBAL);
    if (!lib_handle) {
        puts("cannot load side module");
    }

    //setenv("HOME","/data/data/u.root.upy",0);
    setenv("HOME","/",1);

    //setenv("MICROPYPATH","/data/data/u.root.upy/assets",0);
    setenv("MICROPYPATH","/",1);

    //printf("Py_InitializeEx\n");
    Py_InitializeEx(0);

    for (int i=0; i<argc; i++) {
        fprintf(stderr,"Micropython-wasm argv[%d]='%s'\n",i,argv[i]);
        if (i>1)  // skip silly "./this.program" and sys.executable
            mp_obj_list_append(mp_sys_argv, MP_OBJ_NEW_QSTR(qstr_from_str(argv[i])));
    }

    //chdir("/data/data/u.root.upy/assets");
    chdir("/");

    fprintf(stderr," =================================================\n");
    PyArg_ParseTuple(nullptr,"%s\n","argv1");
    fprintf(stderr," =================================================\n");

    writecode(
        "boot.py",
        "import sys;"
        "sys.path.clear();"
        "sys.path.append( '' )\n"
        "import sys;print(sys.implementation.name,'%s.%s.%s' % sys.implementation.version, sys.version, sys.platform)\n"
    );

    PyRun_VerySimpleFile("/boot.py");



    emscripten_set_main_loop( py_iter_one, 0, 1);  // <= this will exit to js now.

}

#else








#endif
