#define DBG 0
#define DLOPEN 0



#define clog(...) if (show_os_loop(-1)){ fprintf(stderr, __VA_ARGS__ );fprintf(stderr, "\n"); }

#define SYS_MAX_RECURSION 32

#include "core/main_pre.c"

#include <setjmp.h>


// to have correct pointer cast on .call those must be included here
#include "core/objfun.c"
#include "core/objclosure.c"
#include "core/objboundmeth.c"
#include "core/objgenerator.c"
#include "core/objtype.c"


#include "py/runtime.h"
#include "core/vm.c"



EMSCRIPTEN_KEEPALIVE int VM_depth = 0;

extern mp_obj_t mp_builtin___import__(size_t n_args, const mp_obj_t *args);


#include "vm/gosub.c"

static mp_lexer_t *lex;

#define VM_DECODE_UINT \
    mp_uint_t unum = 0; \
    do { \
        unum = (unum << 7) + (*CTX.ip & 0x7f); \
    } while ((*CTX.ip++ & 0x80) != 0)

#define VM_DECODE_ULABEL CTX.ulab = (CTX.ip[0] | (CTX.ip[1] << 8)); CTX.ip += 2
#define VM_DECODE_SLABEL CTX.slab = (CTX.ip[0] | (CTX.ip[1] << 8)) - 0x8000; CTX.ip += 2

#define VM_DECODE_QSTR \
    CTX.qst = CTX.ip[0] | CTX.ip[1] << 8; \
    CTX.ip += 2;

#define VM_DECODE_PTR \
    VM_DECODE_UINT; \
    CTX.ptr = (void*)(uintptr_t)CTX.code_state->fun_bc->const_table[unum]

#define VM_DECODE_OBJ \
    VM_DECODE_UINT; \
    mp_obj_t obj = (mp_obj_t)CTX.code_state->fun_bc->const_table[unum]



#include "vm/debug.c"



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

#include "vm/stackwith.c"

void
py_iter_one(void) {

    if ( CTX.vmloop_state >= VM_PAUSED )
        CTX.vmloop_state--;


    if (CTX.vmloop_state <= VM_RESUMING) {
        crash_point = &&VM_CRASH;

        if ( (ENTRY_POINT != JMP_NONE)  && !JUMPED_IN) {
            fprintf(stderr,"spawn vm %d entry => %d\n", ctx_current, CTX.pointer);

VM_jump_table_entry: {
                void* jump_entry;
                jump_entry = ENTRY_POINT;
                // Never to re-enter as this point. can only use the exit.
                JUMPED_IN = 1;
                goto *jump_entry;
            }
        }


        if (EXIT_POINT != JMP_NONE) {
            fprintf(stderr,"resuming vm %d ptr=%d\n", ctx_current, CTX.pointer);

VM_jump_table_exit: {
                // was it gosub
                if (JUMP_TYPE == TYPE_SUB)
                    RETURN;

                // was it branching
                if (JUMP_TYPE == TYPE_JUMP)
                    COME_FROM;

                FATAL("VM_jump_table_exit: invalid jump table");
            }
        }

    } else {
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

        if (show_os_loop(-1)) {
            clog(" -------------------- BEGIN --------------------");
            //fun_ptr();
        }

        //fprintf(stderr,"IO %lu [%s]\n", strlen(repl_line), repl_line);
        //is it async top level ?
        if (endswith(repl_line, "#async-tl")) {
            fprintf(stderr, "#async-tl -> aio.asyncify()\n");
            PyRun_SimpleString("aio.asyncify()");

        } else {

            if (prepare_code()) {

                static nlr_buf_t main_nlr;

                if (nlr_push(&main_nlr) == 0) {

                    CTX.parse_tree = mp_parse(CTX.lex, MP_PARSE_FILE_INPUT);

                    mpi_ctx[ctx_current].self_in = mp_compile(
                            &CTX.parse_tree,
                            CTX.source_name,
                            MP_EMIT_OPT_NONE,
                            EXEC_FLAG_IS_REPL);

                    //CTX.type = mp_obj_get_type(CTX.self_in);

                    #include "vm/stackless.c"

                    #include "vm/stackmess.c"

                    nlr_pop();
                } else {
                    // uncaught exception
                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)main_nlr.ret_val);
                    KPANIC = 1;
                }
            }

            //?? TODO:CTX CTX.vmloop_state = VM_IDLE;
            repl_line[0]=0;
            if (show_os_loop(0)) {
                fprintf(stderr," ----------- END -------------------\n");
            }
        }

    }

    // call asyncio auto stepping first in case of no repl
   // if (!KPANIC)
     //   PyRun_SimpleString("aio.step()");

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

VM_CRASH:
    clog("KP - game over");
    //emscripten_pause_main_loop();
    emscripten_cancel_main_loop();
    return;

VM_paused:
    CTX.vmloop_state = VM_PAUSED + 3;
    fprintf(stderr," - paused interpreter %d -\n", ctx_current);
    return;

VM_syscall:
    CTX.vmloop_state = VM_SYSCALL;
    if (show_os_loop(-1))
        fprintf(stderr," - syscall %d -\n", ctx_current);
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
