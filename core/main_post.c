

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

int
await_dlopen(const char *def){
    return !EM_ASM_INT( { return defined(UTF8ToString($0), window.lib); }, def );
}

// should check null

EMSCRIPTEN_KEEPALIVE char*
shm_ptr() {
    return &repl_line[0];
}

EMSCRIPTEN_KEEPALIVE int
repl_run(int warmup) {
    if (warmup==1)
        return REPL_INPUT_MAX;
    pyexec_event_repl_init();
    repl_started = REPL_INPUT_MAX;
    return 1;
}


char **copy_argv(int argc, char *argv[]) {
  // calculate the contiguous argv buffer size
  int length=0;
  size_t ptr_args = argc + 1;
  for (int i = 0; i < argc; i++)
  {
    length += (strlen(argv[i]) + 1);
  }
  char** new_argv = (char**)malloc((ptr_args) * sizeof(char*) + length);
  // copy argv into the contiguous buffer
  length = 0;
  for (int i = 0; i < argc; i++)
  {
    new_argv[i] = &(((char*)new_argv)[(ptr_args * sizeof(char*)) + length]);
    strcpy(new_argv[i], argv[i]);
    length += (strlen(argv[i]) + 1);
  }
  // insert NULL terminating ptr at the end of the ptr array
  new_argv[ptr_args-1] = NULL;
  return (new_argv);
}


int
main(int argc, char *argv[]) {

    //keep symbol global for wasm debugging
    //fprintf(stderr,"//#FIXME: add sys.executable to sys\n");

    void *lib_handle = dlopen("lib/libtest.wasm", RTLD_NOW | RTLD_GLOBAL);
    if (!lib_handle) {
        puts("cannot load side module");
    } else
        puts("+1 browser can dlopen side wasm library");


    lib_handle = dlopen("/lib/libmicropython.so", RTLD_NOW | RTLD_GLOBAL);
    if (!lib_handle) {
        puts("-1 browser can't dlopen main dynamically : linking error or wasm 4KB limitation");
    } else
        puts("+1 browser can dlopen main wasm library");

    // NOT USING dlopen() and libmicropython.wasm ATM because chrome will fail
    // and that would mean compiling to shared library as .js

/*
    Error in loading dynamic library libmicropython.wasm:
    RangeError: WebAssembly.Compile is disallowed on the main thread, if the buffer size is larger than 4KB.
    Use WebAssembly.compile, or compile on a worker thread.
*/

    //setenv("HOME","/data/data/u.root.upy",0);
    setenv("HOME","/",1);

    //setenv("MICROPYPATH","/data/data/u.root.upy/assets",0);
    setenv("MICROPYPATH","/",1);

    g_argc = argc;
    g_argv = copy_argv(argc, argv);

    //py_init(argc, argv);

    entry_point[0]=JMP_NONE;
    exit_point[0]=JMP_NONE;
    come_from[0]=0;

    for (int i=0; i<SYS_MAX_RECURSION; i++)
        mp_new_interpreter(&mpi_ctx, i, 0 , 0);

    // 0 hypervisor with no branching ( can use optimized original vm.c )
    // 1 supervisor
    // 2 __main__

    mp_new_interpreter(&mpi_ctx, 1, 0, 2);

    // 2 has no parent for now, just back to OS
    mp_new_interpreter(&mpi_ctx, 2, 0, 0);

    ctx_current = 1;
    while ( mpi_ctx[ctx_current].childcare )
        ctx_current = (int)mpi_ctx[ctx_current].childcare;

    fprintf(stdout,"running __main__ on pid=%d\n", ctx_current);

    emscripten_set_main_loop( py_iter_one, 0, 1);  // <= this will exit to js now.

    // unreachable;
    return 0;
}
