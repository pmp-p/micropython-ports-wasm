#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fdfile.h"

#include "py/compile.h"
#include "py/emitglue.h"
#include "py/objtype.h"
#include "py/runtime.h"
#include "py/parse.h"
#include "py/bc0.h"
#include "py/bc.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"


// for mp_call_function_0
#include "py/parsenum.h"
#include "py/compile.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/objlist.h"
#include "py/objmodule.h"  // <= function defined in
#include "py/objgenerator.h"
#include "py/smallint.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include "py/stackctrl.h"
#include "py/gc.h"












#include "emscripten.h"

/*
CFLAGS="-Wfatal-errors -Wall -Wextra -Wunused -Werror -Wno-format-extra-args -Wno-format-zero-length\
 -Winit-self -Wimplicit -Wimplicit-int -Wmissing-include-dirs -Wswitch-default -Wswitch-enum\
 -Wunused-parameter -Wdouble-promotion -Wchkp -Wno-coverage-mismatch -Wstrict-overflow\
 -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wnonnull -Wnonnull-compare\
 -Wnull-dereference -Wignored-qualifiers -Wignored-attributes\
 -Wmain -Wpedantic -Wmisleading-indentation -Wmissing-braces -Wmissing-include-dirs\
 -Wparentheses -Wsequence-point -Wshift-overflow=2 -Wswitch -Wswitch-default -Wswitch-bool\
 -Wsync-nand -Wunused-but-set-parameter -Wunused-but-set-variable -Wunused-function -Wunused-label\
 -Wunused-parameter -Wunused-result -Wunused-variable -Wunused-const-variable=2 -Wunused-value\
 -Wuninitialized -Winvalid-memory-model -Wmaybe-uninitialized -Wstrict-aliasing=3\
 -Wsuggest-attribute=pure -Wsuggest-attribute=const\
 -Wsuggest-attribute=noreturn -Wsuggest-attribute=format -Wmissing-format-attribute\
 -Wdiv-by-zero -Wunknown-pragmas -Wbool-compare -Wduplicated-cond\
 -Wtautological-compare -Wtrampolines -Wfloat-equal -Wfree-nonheap-object -Wunsafe-loop-optimizations\
 -Wpointer-arith -Wnonnull-compare -Wtype-limits -Wcomments -Wtrigraphs -Wundef\
 -Wendif-labels -Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings -Wclobbered\
 -Wconversion -Wdate-time -Wempty-body -Wjump-misses-init -Wsign-compare -Wsign-conversion\
 -Wfloat-conversion -Wsizeof-pointer-memaccess -Wsizeof-array-argument -Wpadded -Wredundant-decls\
 -Wnested-externs -Winline -Wbool-compare -Wno-int-to-pointer-cast -Winvalid-pch -Wlong-long\
 -Wvariadic-macros -Wvarargs -Wvector-operation-performance -Wvla -Wvolatile-register-var\
 -Wpointer-sign -Wstack-protector -Woverlength-strings -Wunsuffixed-float-constants\
 -Wno-designated-init -Whsa\
 -march=x86-64 -m64 -Wformat=2 -Warray-bounds=2 -Wstack-usage=120000 -Wstrict-overflow=5 -fmax-errors=5 -g\
 -std=c99 -D_POSIX_SOURCE -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=700 -pedantic-errors"
*/

#define DBG 0
#define DLOPEN 0

#define clog(...) if (show_os_loop(-1)){ fprintf(stderr, __VA_ARGS__ );fprintf(stderr, "\n"); }

#define SYS_MAX_RECURSION 32

static int g_argc;
static char **g_argv; //[];

size_t
bsd_strlen(const char *str)
{
        const char *s;
        for (s = str; *s; ++s);
        return (s - str);
}






/* =====================================================================================
    bad sync experiment with file access trying to help on
        https://github.com/littlevgl/lvgl/issues/792

    status: better than nothing.
*/

#include "wasm_file_api.c"
#include "wasm_import_api.c"

static int KPANIC = 0;







char **copy_argv(int argc, char *argv[]) {
  // calculate the contiguous argv buffer size
  int length=0;
  size_t ptr_args = argc + 1;
  for (int i = 0; i < argc; i++)
  {
    length += (bsd_strlen(argv[i]) + 1);
  }
  char** new_argv = (char**)malloc((ptr_args) * sizeof(char*) + length);
  // copy argv into the contiguous buffer
  length = 0;
  for (int i = 0; i < argc; i++)
  {
    new_argv[i] = &(((char*)new_argv)[(ptr_args * sizeof(char*)) + length]);
    strcpy(new_argv[i], argv[i]);
    length += (bsd_strlen(argv[i]) + 1);
  }
  // insert NULL terminating ptr at the end of the ptr array
  new_argv[ptr_args-1] = NULL;
  return (new_argv);
}



#include "upython.c"


EMSCRIPTEN_KEEPALIVE int
repl_run(int warmup) {
    if (warmup==1)
        return MPI_SHM_SIZE;
    //Py_NewInterpreter();
    repl_started = MPI_SHM_SIZE;
    return 1;
}

#if 0
int endswith(const char * str, const char * suffix)
{
  int str_len = bsd_strlen(str);
  int suffix_len = bsd_strlen(suffix);

  return
    (str_len >= suffix_len) && (0 == strcmp(str + (str_len-suffix_len), suffix));
}

#else
int endswith(const char * str, const char * suffix)
{
  int str_len = strlen(str);
  int suffix_len = strlen(suffix);

  return
    (str_len >= suffix_len) && (0 == strcmp(str + (str_len-suffix_len), suffix));
}
#endif


#include "vmsl/vmreg.h"

#include "vmsl/vmreg.c"

extern mp_obj_t fun_bc_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args);
extern mp_vm_return_kind_t mp_execute_bytecode(mp_code_state_t *code_state, volatile mp_obj_t inject_exc);
extern mp_obj_t gen_instance_iternext(mp_obj_t self_in);

#include <search.h>
void
main_loop_or_step(void) {
call_:;
    if (VMOP < VMOP_WARMUP) {
        if (VMOP < VMOP_INIT) {
            puts("init");
            crash_point = &&VM_stackmess;
            Py_Initialize();
            Py_NewInterpreter();

            VMOP = VMOP_INIT;
            //fun_ptr();
            entry_point[0]=JMP_NONE;
            exit_point[0]=JMP_NONE;
            come_from[0]=0;
            type_point[0]=0;

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
          //  fprintf(stdout,"    PTR fun_bc_call==%p\n", &fun_bc_call );
            //fprintf(stdout,"    PTR mp_execute_bytecode==%p\n", &mp_execute_bytecode );
            return;
        } // no continuation -> syscall

        if (VMOP==VMOP_INIT) {
            VMOP = VMOP_WARMUP;
            puts("test");

            PyRun_SimpleString("import site");
            //PyRun_SimpleString("print('Hello Emscripten')");

            show_os_loop(1);
            //strcpy( i_main.shm_stdio ,"print('Hello Stackless')");
            //strcpy( i_main.shm_stdio , "print('Hello Stackless');import pystone;print(pystone)");
            strcpy( i_main.shm_stdio , "print('Hello Stackless');import pystone;pystone.main()");
            JUMP( def_PyRun_SimpleString, "main_loop_or_step");
            puts("-resuming-");
            return;
        }
    }

    if ( (ENTRY_POINT != JMP_NONE)  && !JUMPED_IN) {
        fprintf(stderr,"re-enter-on-entry %d => %d\n", ctx_current, CTX.pointer);
        void* jump_entry;
        jump_entry = ENTRY_POINT;
        // Never to re-enter as this point. can only use the exit.
        JUMPED_IN = 1;
        goto *jump_entry;
    }

    // allow that here ?
    if ( (EXIT_POINT != JMP_NONE)  && JUMPED_IN) {
        fprintf(stderr,"re-enter-on-exit %d => %d\n", ctx_current, CTX.pointer);

        // was it gosub
        if (JUMP_TYPE == TYPE_SUB)
            RETURN;

        // was it branching
        if (JUMP_TYPE == TYPE_JUMP)
            COME_FROM;
    }

#define io_stdin i_main.shm_stdio
    if (io_stdin[0]) {

        //is it async top level ? let python access shared mem and rewrite code
        if (endswith(io_stdin, "#async-tl")) {
            fprintf(stderr, "#async-tl -> aio.asyncify()\n");
            PyRun_SimpleString("aio.asyncify()");
        } else {
            //then it is sync top level
            JUMP( def_PyRun_SimpleString, "main_loop_or_step");
            //do_str(i_main.shm_stdio, MP_PARSE_FILE_INPUT);
        }
        // mark done
        io_stdin[0] = 0;
    }
#undef io_stdin


    while (!KPANIC) {
        if (!rbb_is_empty(&out_rbb)) {
            // flush stdout
            unsigned char out_c = 0;
            printf("{\"%c\":\"",49);
            //TODO put a 0 at end and printf buffer directly
            while (rbb_pop(&out_rbb, &out_c))
                printf("%c", out_c );
            printf("\"}\n");
        }

        if (VMOP==VM_H_CF_OR_SC)
            goto VM_stackmess;

        char *keybuf ;
        keybuf = shm_get_ptr(IO_KBD, 0) ;

        // only when scripting interface is idle and repl ready
        while (repl_started) {
            // should give a way here to discard repl events feeding  "await input()" instead
            int rx = (int)keybuf++;
            if (rx) {
                // if (rx==12) fprintf(stderr,"IO(12): Form Feed "); //clear screen
                //if (rx>127) fprintf(stderr, "FIXME:stdin-utf8:%u\n", rx );
                pyexec_event_repl_process_char(rx);
            } else break;
        }
        return;
    }



// def PyRun_SimpleString(const_char_p src) -> int;
def_PyRun_SimpleString: {
//return:
    int ret = 0;
//args:
    char* src = i_main.shm_stdio;
    mp_parse_input_kind_t input_kind = MP_PARSE_FILE_INPUT;
//vars
    nlr_buf_t nlr;

//code
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, false);


        //STACKLESS STARTS HERE
        ctx_get_next(CTX_NEW);

        NEXT.self_in = module_fun;
            GOSUB(def_mp_call_function_0,"PyRun_SimpleString");
        //STACKLESS ENDS HERE


        src[0]=0;
        nlr_pop();
    } else {

        // RETEXC =  ???
        // uncaught exception
        if (mp_obj_is_subclass_fast(mp_obj_get_type((mp_obj_t)nlr.ret_val), &mp_type_SystemExit)) {
            mp_obj_t exit_val = mp_obj_exception_get_value(MP_OBJ_FROM_PTR(nlr.ret_val));
            if (exit_val != mp_const_none) {
                mp_int_t int_val;
                if (mp_obj_get_int_maybe(exit_val, &int_val)) {
                    ret = int_val & 255;
                } else {
                    ret = 1;
                }
            }
        } else {
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
            ret = 1;
        }
    }
    COME_FROM;
} // PyRun_SimpleString


VM_stackmess:
    puts("no guru meditation, bye");
    #if !ASYNCIFY
    emscripten_cancel_main_loop();
    #endif
    return;

//==================================================================

#if __DEV__
    #include "vmsl/unwrap.c"
#endif

//==================================================================

VM_syscall:;
    puts("-syscall-");
}


int
main(int argc, char *argv[]) {

    g_argc = argc;
    g_argv = copy_argv(argc, argv);

    fprintf(stdout,"Hello WASM\n");

#if !ASYNCIFY
    emscripten_set_main_loop( main_loop_or_step, 0, 1);  // <= this will exit to js now.
#else
    while (!KPANIC) {
        emscripten_sleep(1);
        main_loop_or_step();
    }
#endif
    puts("no guru meditation");
    return 0;
}




























//



