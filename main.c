#define DBG 0
#define DLOPEN 0

//order matters
#define VM_IDLE     0
#define VM_RUNNING   1
#define VM_RESUMING  2
#define VM_PAUSED    3


#include "core/main_pre.c"

#include <setjmp.h>

#include "core/objfun.c"

#include "py/runtime.h"
#include "core/vm.c"


struct mp_interpreter {
    // builtins override dict  ?
    // __main__ ?
    // sys.(std*) ?
    // sys.argv

    // cpu time load stats ?

    //who created us
    unsigned int parent ;

    //who did we create and was running last
    unsigned int childcare ;

    int vmloop_state;
    unsigned long coropass;

    mp_lexer_t *lex;
    qstr source_name;
    mp_parse_tree_t parse_tree;

    mp_obj_t * /*const*/ fastn;
    //mp_exc_stack_t * /*const*/ exc_stack;
    mp_exc_stack_t * /*const*/ exc_stack;

    size_t n_state;
    size_t state_size;

    volatile mp_obj_t inject_exc;
    mp_code_state_t *code_state;
    mp_exc_stack_t *volatile exc_sp;

    mp_obj_fun_bc_t *self;
    mp_obj_type_t *type;

    int jump_index;

    mp_obj_t self_in;
    size_t n_args;
    size_t n_kw;
    const mp_obj_t *args;

    mp_obj_t result;

    mp_vm_return_kind_t vm_return_kind;

};

#define SYS_MAX_RECURSION 32

//TODO sys max recursion handling.
static struct mp_interpreter mpi_ctx[32];

static int ctx_current = 1;
static int ctx_next = 2;


static mp_lexer_t *lex;

/*
static int vmloop_state = VM_IDLE;
static unsigned long coropass;
static mp_lexer_t *lex;

static qstr source_name;
static mp_parse_tree_t parse_tree;
static mp_obj_t self_in;
static mp_obj_type_t *type;
static mp_obj_fun_bc_t *self;
static size_t n_state;
static size_t state_size;
static mp_vm_return_kind_t vm_return_kind;
static volatile mp_obj_t inject_exc;
static mp_code_state_t *code_state;
static mp_exc_stack_t *volatile exc_sp;

void ctx_restore(ctx) {
    memcpy(&mpi_ctx[0], (const unsigned char*)&mpi_ctx[ctx], sizeof(mpi_ctx[0]));
}

void ctx_save(ctx) {
    memcpy(&mpi_ctx[ctx], (const unsigned char*)&mpi_ctx[0], sizeof(mpi_ctx[0]));
}
*/

#define CTX  mpi_ctx[ctx_current]
#define NEXT mpi_ctx[ctx_next]

void mp_new_interpreter(void * mpi, int ctx, int parent, int childcare) {
    mpi_ctx[ctx].vmloop_state = VM_IDLE;
    mpi_ctx[ctx].parent = parent;
    mpi_ctx[ctx].childcare = childcare;
    mpi_ctx[ctx].jump_index = 0;
    mpi_ctx[ctx].code_state = NULL ;

    mpi_ctx[ctx].n_args = 0;
    mpi_ctx[ctx].n_kw = 0;
    mpi_ctx[ctx].args = NULL;

}

int ctx_get() {
    int ctx;
    for (ctx=3; ctx<SYS_MAX_RECURSION; ctx++)
        if (mpi_ctx[ctx].vmloop_state == VM_IDLE)
            break;
    return ctx;
}

int ctx_push(int jump_index) {
    //push
    int ctx = ctx_get();

    //track
    CTX.childcare = ctx ;

    mp_new_interpreter(&mpi_ctx, ctx, ctx_current, 0);
    mpi_ctx[ctx].jump_index = jump_index;
    return ctx;
}


int prepare_code() {
    //lib/utils/pyexec.c:#define EXEC_FLAG_IS_REPL (4)
    #define EXEC_FLAG_IS_REPL (4)

    CTX.lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, repl_line, strlen(repl_line), 0);
    // TODO: REMOVE ?
    CTX.source_name = CTX.lex->source_name;

    if (CTX.lex == NULL) {
        printf("152:malloc: lexer %lu\n", strlen(repl_line));
        KPANIC = 1;
        return 0;
    }
    return 1;
}








void
py_iter_one(void) {

    if ( CTX.vmloop_state >= VM_PAUSED ) {

        if (--CTX.vmloop_state == VM_RESUMING) {

            //ctx_restore(ctx_current);
            //vmloop_state = CTX.vmloop_state ;

            fprintf(stderr,"resuming vm %d (trying) jump to %d\n", ctx_current, CTX.jump_index);
            goto VM_jump_table;
        }
        fprintf(stderr," - paused -\n");
        return;
    }

    // shm not ready or boot not finished.
    if (repl_started<-1) {
        #if DLOPEN
        if (py_iter_init_dlopen())
        #else
        if (py_iter_init())
        #endif
            return;
    }


    if (!rbb_is_empty(&out_rbb)) {
        // flush stdout
        unsigned char out_c = 0;
        printf("{\"%c\":\"",49);
        //TODO put a 0 at end and printf buffer directly
        while (rbb_pop(&out_rbb, &out_c))
            printf("%c", out_c );
        printf("\"}\n");
    }


    // io demux will be done here too via io_loop(json_state)
    if (!KPANIC && repl_line[0]){

        if (show_os_loop(-1))
            fprintf(stderr," ----------- BEGIN ------------------\n");


        //fprintf(stderr,"IO %lu [%s]\n", strlen(repl_line), repl_line);
        //is it async top level ?
        if (endswith(repl_line, "#async-tl")) {
            fprintf(stderr, "#async-tl -> asyncio.asyncify()\n");
            PyRun_SimpleString("asyncio.asyncify()");

        } else {

            if (prepare_code()) {

                nlr_buf_t nlr;
                if (nlr_push(&nlr) == 0) {

                    CTX.parse_tree = mp_parse(CTX.lex, MP_PARSE_FILE_INPUT);

                    mpi_ctx[ctx_current].self_in = mp_compile(
                            &CTX.parse_tree,
                            CTX.source_name,
                            MP_EMIT_OPT_NONE,
                            EXEC_FLAG_IS_REPL);

                    CTX.type = mp_obj_get_type(CTX.self_in);

                    #include "core/itervm.c"

                    nlr_pop();
                } else {
                    // uncaught exception
                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                    KPANIC = 1;
                }
            }
VM_idle:
            CTX.vmloop_state = VM_IDLE;
            repl_line[0]=0;
            if (show_os_loop(-1)) {
                fprintf(stderr," ----------- END -------------------\n");
                show_os_loop(0);
            }
        }

    }

    // call asyncio auto stepping first in case of no repl
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
            // if (rx==12) fprintf(stderr,"IO(12): Form Feed "); //clear screen
            //if (rx>127) fprintf(stderr, "FIXME:stdin-utf8:%u\n", rx );
            pyexec_event_repl_process_char(rx);
        } else break;
    }

VM_ret:
     return;

VM_paused:
    //ctx_save(ctx_current);
    CTX.vmloop_state = VM_PAUSED + 5;
    fprintf(stderr," - paused interpreter %d -\n", ctx_current);
}

/* =====================================================================================
    bad sync experiment with file access trying to help on
        https://github.com/littlevgl/lvgl/issues/792

    status: better than nothing.
*/

#include "wasm_file_api.c"
#include "wasm_import_api.c"

//=====================================================================================


#include "core/main_post.c"
