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

static int repl_started = 0;

void
py_iter_one(void){
    if (repl_line[0]){
        PyRun_SimpleString(repl_line);
        repl_line[0]=0;
    }

    if (!repl_started) return;

    int rx = EM_ASM_INT({
if (window.stdin_array.length)
    return window.stdin_array.shift();
    return 0;
});
    if (rx) {
        if (rx>127)
            fprintf(stderr, "FIXME:stdin-utf8:%u\n", rx );
        pyexec_event_repl_process_char(rx);
    }

}

/* =====================================================================================
    bad sync experiment with file access trying to help on
    https://github.com/littlevgl/lvgl/issues/792

*/


// VERY BAD
int hack_open(const char *url) {
    fprintf(stderr,"204:hack_open[%s]\n", url);
    if (url[0]==':') {
        fprintf(stderr,"  -> same host[%s]\n", url);
        int fidx = EM_ASM_INT({return hack_open(Pointer_stringify($0)); }, url );
        char fname[256];
        snprintf(fname, sizeof(fname), "cache_%d", fidx);
        return fileno( fopen(fname,"r") );
    }

    return 0;
}

// BAD ENOUGH
mp_import_stat_t hack_modules(const char *modname) {
    if( access( modname, F_OK ) != -1 ) {
        struct stat stats;
        stat(modname, &stats);
        if (S_ISDIR(stats.st_mode))
            return MP_IMPORT_STAT_DIR;
        return MP_IMPORT_STAT_FILE;
    }
    //FIXME: directory vs files ?
    int found = EM_ASM_INT({return file_exists(Pointer_stringify($0), true); }, modname ) ;
    if ( found ) {
        int dl = EM_ASM_INT({return hack_open(Pointer_stringify($0),Pointer_stringify($0)); }, modname );
        fprintf(stderr,"hack_modules: dl %s size=%d", modname, dl);
        if (dl)
            return MP_IMPORT_STAT_FILE;
    }
    fprintf(stderr,"404:hack_modules '%s' (%d)\n", modname, found);
    return MP_IMPORT_STAT_NO_EXIST;
}

//=====================================================================================




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


EMSCRIPTEN_KEEPALIVE void
repl_init(){
    if (repl_started) return;
    pyexec_event_repl_init();
    repl_started=1;
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
        "import sys\n"
        "print(sys.implementation.name,'%s.%s.%s' % sys.implementation.version, sys.version, sys.platform)\n"
        "sys.path.clear()\n"
        "sys.path.append( '' )\n"
//        "import sys\n"
        "def run(file):\n"
        "    if not file.endswith('.py'): file+='.py'\n"
        "    print(file)\n"
        "    code = compile( open(file).read(), file, 'exec')\n"
        "    exec( code, globals(), globals())\n"
        "\n"
        "\n"
        "\n"

    );

    PyRun_VerySimpleFile("/boot.py");



    emscripten_set_main_loop( py_iter_one, 0, 1);  // <= this will exit to js now.

}

#else








#endif
