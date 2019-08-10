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


#include "vm/stackwith.c"
#include "core/modbuiltins.c" // mpsl_iternext_allow_raise is in stackwith.c


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

static int coropass =0 ;

size_t
bsd_strlen(const char *str)
{
        const char *s;
        for (s = str; *s; ++s);
        return (s - str);
}

int code_prepare() {
    //lib/utils/pyexec.c:#define EXEC_FLAG_IS_REPL (4)

    return 1;
}

void code_free(){

}

void repl_done(){
    //mark repl buffer consummed as a null str
    repl_line[0] = 0;}


typedef struct _mp_reader_mem_t {
    size_t free_len; // if >0 mem is freed on close by: m_free(beg, free_len)
    const byte *beg;
    const byte *cur;
    const byte *end;
} mp_reader_mem_t;

void
py_iter_one(void) {

    if (VMOP<0) {
        crash_point = &&VM_stackmess;
        py_iter_init();
        VMOP = VMOP_NONE;
        fun_ptr();

        return;
    }

    if ( CTX.vmloop_state >= VM_PAUSED )
        CTX.vmloop_state--;


    if (CTX.vmloop_state > VM_RESUMING) {
        fprintf(stderr," - paused -\n");
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





    //fprintf(stderr,"loop(repl=%d, panic=%d, code=%lu)\n", repl_started, KPANIC, bsd_strlen(repl_line) );

    if (KPANIC)
        return;

    if (!repl_started)
        return;

    // io demux will be done here too via io_loop(json_state)

    //is it async top level ?
    if (endswith(repl_line, "#async-tl")) {
        fprintf(stderr, "#async-tl -> aio.asyncify()\n");
        PyRun_SimpleString("aio.asyncify()");
        repl_done();
        return;
    }



    if ( (ENTRY_POINT != JMP_NONE)  && !JUMPED_IN) {
        fprintf(stderr,"spawn vm %d entry => %d\n", ctx_current, CTX.pointer);
        void* jump_entry;
        jump_entry = ENTRY_POINT;
        // Never to re-enter as this point. can only use the exit.
        JUMPED_IN = 1;
        goto *jump_entry;
    }


    if (!bsd_strlen(repl_line))
        return;


    if (endswith(repl_line, "#aio.step\n")) {
        fprintf(stderr,"loop(AIO)\n" );
        // running repl after script in cpython is sys.flags.inspect, should monitor and init repl

        // only when scripting interface is idle and repl ready
        while (repl_started) {
            // should give a way here to discard repl events feeding  "await input()" instead
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

        // WORKAROUND the ~1K strlen crash
        return;
    } else
        fprintf(stderr,"loop(code=%lu)\n", bsd_strlen(repl_line) );

    #define EXEC_FLAG_IS_REPL (4)
    mp_reader_new_mem(&CTX.reader, (const byte*)repl_line, bsd_strlen(repl_line), 0);
    CTX.lex = mp_lexer_new(MP_QSTR__lt_stdin_gt_, CTX.reader);

    if (CTX.lex == NULL) {
        printf("152:malloc: lexer %lu\n", bsd_strlen(repl_line));
        KPANIC = 1;
        return;
    }

    // TODO where/when call aio.step() if no repl ?
    // call asyncio auto stepping first in case of no repl
    //   PyRun_SimpleString("aio.step()");
    // actually plink does the call.

#if __WASM__
#error unsupported
#else
    static nlr_buf_t main_nlr;

    if (nlr_push(&main_nlr) != 0) {
        // uncaught exception
        clog("194: uncaught exception !");
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)main_nlr.ret_val);
        KPANIC = 1;
        return;
    }
#endif

    CTX.parse_tree = mp_parse(CTX.lex, MP_PARSE_FILE_INPUT);


    clog(" -------------------- BEGIN --------------------");
    //mp_raw_code_t *rc = mp_compile_to_raw_code(parse_tree, source_file, emit_opt, is_repl);
    mp_raw_code_t *rc = mp_compile_to_raw_code(&CTX.parse_tree, CTX.source_name, MP_EMIT_OPT_NONE, false);
    CTX.self_in = mp_make_function_from_raw_code(rc, MP_OBJ_NULL, MP_OBJ_NULL);

// =======================================================================================
// Begin : fun_bc_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args)
VM_fun_bc_call:
    clog("mpsl:%d fun_bc_call", ctx_current);

    #include "vm/stackless.c"

#if __WASM__
#error unsupported
#else
    nlr_pop();
#endif

    if (EXIT_POINT != JMP_NONE) {
        fprintf(stderr,"resuming vm %d ptr=%d\n", ctx_current, CTX.pointer);
        // was it gosub
        if (JUMP_TYPE == TYPE_SUB)
            RETURN;

        // was it branching
        if (JUMP_TYPE == TYPE_JUMP)
            COME_FROM;

        //FATAL("VM_jump_table_exit: invalid jump table");
    }

    if (show_os_loop(0)) {
        fprintf(stderr," ----------- END -------------------\n");
        coropass = 1;
        //?? TODO:CTX CTX.vmloop_state = VM_IDLE;
        clear_shared_array_buffer();
    }

    //mark repl buffer consummed
    repl_line[0] = 0;

    #include "vm/stackmess.c"
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
