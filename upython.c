//
// core/main.c
// core/vfs.c
//

#define MPI_SHM_SIZE 32768

#define MP_IO_SIZE 1024
#define MP_IO_ADDR (MPI_SHM_SIZE - MP_IO_SIZE)


#define IO_KBD 0

#if MICROPY_ENABLE_PYSTACK
    #define MP_STACK_SIZE 65536
    static mp_obj_t pystack[MP_STACK_SIZE];
#endif

static char *stack_top;

#include "upython.h"

#include "core/ringbuf_o.h"
#include "core/ringbuf_b.h"

RBB_T( out_rbb, 4096);


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


struct
PyInterpreterState {
    char *shm_stdio ;
    char *shm_input_event_0;
};


struct
PyThreadState {
    struct PyInterpreterState *interp;
};

static struct PyInterpreterState i_main ;
static struct PyThreadState i_state ;


// should check null
char*
shm_ptr() {
    return &i_main.shm_stdio[0];
}

char*
shm_get_ptr(int major,int minor) {
    // keyboards
    if (major==IO_KBD) {
        if (minor==0)
            return &i_main.shm_stdio[MP_IO_ADDR];
    }
}



char*
Py_NewInterpreter() {
    i_main.shm_stdio = (char *)malloc(MPI_SHM_SIZE);
    i_main.shm_stdio[0] = 0 ;

    i_state.interp = & i_main;
    pyexec_event_repl_init();
    return shm_ptr();
}

void
Py_Initialize() {
    int stack_dummy;
    int heap_size = 256 * 1024 * 1024 ;
    stack_top = (char*)&stack_dummy;

    #if MICROPY_ENABLE_GC
    char *heap = (char*)malloc(heap_size * sizeof(char));
    gc_init(heap, heap + heap_size);
    #endif

    #if MICROPY_ENABLE_PYSTACK
    mp_pystack_init(pystack, &pystack[MP_ARRAY_SIZE(pystack)]);
    #endif

    mp_init();

    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
    mp_obj_list_init(mp_sys_argv, 0);
}

#include "port/asmjs-nlr.c"

#include "wasm_mphal.c"


int
PyRun_SimpleString(const char* command) {
    strcpy( i_main.shm_stdio , command);
    int retval = do_str(i_main.shm_stdio, MP_PARSE_FILE_INPUT);
    i_main.shm_stdio[0] = 0;
    return retval;
}
