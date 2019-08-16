// emcc -s WASM=1 -o bugrepro.js bugrepro.c

/*
reset;
. /opt/sdk/emsdk/emsdk_env.sh
rm bugrepro.js
if echo $@|grep -q ASYNC
then
    emcc -s WASM=1 -o bugrepro.js bugrepro.c -DASYNCIFY=1 -s ASYNCIFY
else
    emcc -s WASM=1 -o bugrepro.js bugrepro.c
fi
node bugrepro.js
*/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>

#include "emscripten.h"


#include "vmsl/vmconf.h"
#include "vmsl/vmreg.c"



void
main_loop_or_step(void) {

    if (VMOP<0) {
        puts("init");
        crash_point = &&VM_stackmess;
        //Py_Initialize();
        VMOP = VMOP_NONE;
        //fun_ptr();
        #if !ASYNCIFY
        return;
        #endif
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

VM_fun_bc_call:;
    for (;;) {



        // do mostly  C stack stuff

        // ....

        // or bounce

#if ASYNCIFY
        #pragma message ("    emscripten_sleep     ")
        emscripten_sleep(100);
        // REACHABLE
#else
        SYSCALL(VM_syscall, VMOP_NONE, VM_resume, "FUN_NAME");
        // UNREACHABLE
#endif

        // js had some time
        // can now check async stuff
VM_resume:;

        if (check_timer()) {
            printf("timer");

// call some unwrapped function without a C stack

            GOSUB(VM_sub, VM_sub_done, "FUN_NAME") ;
VM_sub_done:

            printf(" happened! should now exit\n");
            #if ASYNCIFY
                //goto HELP_WHERE_IS_THE_EXIT;
                break;
                //return;

            #else
                emscripten_cancel_main_loop();
            #endif

        } else puts("waiting");



    }

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

HELP_WHERE_IS_THE_EXIT:;
    puts("bye");
    return;

VM_sub:
    printf(" < has > ");
    RETURN;


VM_stackmess:

    if (VMOP==VMOP_NONE)
        return;

    if (VMOP==VMOP_CRASH) {
        clog("KP - game over");
        emscripten_cancel_main_loop();
        return;
    }

    if (VMOP==VMOP_PAUSE) {
        //CTX.vmloop_state = VM_PAUSED + 3;
        clog(" - paused interpreter %d -\n", ctx_current);
        return;
    }


    if (VMOP==VMOP_SYSCALL) {
        //CTX.vmloop_state = VM_SYSCALL;
        clog(" - syscall %d -\n", ctx_current);
        return;
    }


    if (VMOP==VMOP_CALL) {
        if (cond1) {
            ctx_sti++;

        }

        if ( ctx_sti>0)
            BRANCH(VM_fun_bc_call, VMOP_NONE, CTX_call_function_n_kw_cli, FUN_NAME);

        BRANCH(VM_fun_bc_call, VMOP_NONE, CTX_call_function_n_kw_resume, FUN_NAME);

        //no continuation
    }


CTX_call_function_n_kw_cli:;
    ctx_sti--;

CTX_call_function_n_kw_resume:;
        return;



VM_syscall:;

}


// start_timer(): call JS to set an async timer for 500ms
EM_JS(void, start_timer, (), {
  Module.timer = false;
  setTimeout(function() {
    Module.timer = true;
  }, 500);
});



#if ASYNCIFY
int
main(int argc, char *argv[]) {
    puts("USING ASYNCIFY UPSTREAM");

    start_timer();

    // "Infinite loop", synchronously poll for the timer.
    main_loop_or_step();

    return 0;
}
#else
int
main(int argc, char *argv[]) {
    puts("fastcomp");

    start_timer();

    emscripten_set_main_loop( main_loop_or_step, 0, 1);  // <= this will exit to js now.
    // unreachable;

    return 0;
}

#endif



