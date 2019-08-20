#pragma message ("__stackless testing___")
/*
mp_obj_t mp_call_function_0(mp_obj_t fun) {
    return mp_call_function_n_kw(fun, 0, 0, NULL);
}
*/

def_mp_call_function_0: {
    ctx_get_next(CTX_NEW);
    NEXT.self_in = CTX.self_in;
    NEXT.n_args = 0;
    NEXT.n_kw = 0;
    NEXT.args = NULL ;
    RETVAL = MP_OBJ_NULL;
        GOSUB(def_mp_call_function_n_kw,"mp_call_function_0");
    RETVAL = SUBVAL;
    RETURN;
}



def_mp_call_function_n_kw: {
    mp_obj_type_t *type = mp_obj_get_type(CTX.self_in);

    if (type->call != NULL) {

        if ( (int)*type->call == (int)&fun_bc_call ) {
            ctx_get_next(CTX_COPY);
                GOSUB(def_func_bc_call, "mp_call_function_n_kw");
            RETVAL = CTX.sub_value;

        } else
            RETVAL = type->call(CTX.self_in, CTX.n_args, CTX.n_kw, CTX.args);

    } else {
        if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
            mp_raise_TypeError("object not callable");
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                "'%s' object isn't callable", mp_obj_get_type_str(CTX.self_in)));
        }
    }

    RETURN;
}



#if !MICROPY_ENABLE_PYSTACK
    #error "need MICROPY_ENABLE_PYSTACK (1)"
#endif

#if !MICROPY_PERSISTENT_CODE
    #pragma message"VM_DECODE_QSTR/VM_DECODE_PTR/VM_DECODE_OBJ"
#endif


// stores ip pointing to last opcode
#define VM_MARK_EXC_IP_GLOBAL() { CTX.code_state->ip = CTX.ip; }

#define MARK_EXC_IP_SELECTIVE()

// nlr_raise needs to be implemented as a goto, so that the C compiler's flow analyser
// sees that it's possible for us to jump from the VM_DISPATCH loop to the exception
// handler.  Without this, the code may have a different stack layout in the VM_DISPATCH
// loop and the exception handler, leading to very obscure bugs.


#define RAISE(o) do { nlr_pop(); nlr.ret_val = MP_OBJ_TO_PTR(o); goto exception_handler; } while (0)

#define EX_DECODE_ULABEL size_t ulab = (exip[0] | (exip[1] << 8)); exip += 2

#define VM_PUSH_EXC_BLOCK(with_or_finally) do { \
    VM_DECODE_ULABEL; /* except labels are always forward */ \
    ++CTX.exc_sp; \
    CTX.exc_sp->handler = CTX.ip + CTX.ulab; \
    CTX.exc_sp->val_sp = MP_TAGPTR_MAKE(CTX.sp, ((with_or_finally) << 1)); \
    CTX.exc_sp->prev_exc = NULL; \
} while (0)

#define VM_POP_EXC_BLOCK() \
    CTX.exc_sp--; /* pop back to previous exception handler */ \
    CLEAR_SYS_EXC_INFO() /* just clear sys.exc_info(), not compliant, but it shouldn't be used in 1st place */


#define DECODE_CODESTATE_SIZE(bytecode, n_state_out_var, state_size_out_var) \
    { \
        /* bytecode prelude: state size and exception stack size */               \
        n_state_out_var = mp_decode_uint_value(bytecode);                         \
        size_t n_exc_stack = mp_decode_uint_value(mp_decode_uint_skip(bytecode)); \
                                                                                  \
        /* state size in bytes */                                                 \
        state_size_out_var = n_state_out_var * sizeof(mp_obj_t)                   \
                           + n_exc_stack * sizeof(mp_exc_stack_t);                \
    }

#define INIT_CODESTATE(code_state, _fun_bc, n_args, n_kw, args) \
    code_state->fun_bc = _fun_bc; \
    code_state->ip = 0; \
    mp_setup_code_state(code_state, n_args, n_kw, args); \
    code_state->old_globals = mp_globals_get();

/*
def_func_bc_call: {
    RETVAL = (mp_obj_get_type(CTX.self_in))->call(CTX.self_in, CTX.n_args, CTX.n_kw, CTX.args);
    RETURN;
}*/

def_func_bc_call: {
    #if MICROPY_PY_THREAD_GIL && MICROPY_PY_THREAD_GIL_VM_DIVISOR
    // This needs to be volatile and outside the VM loop so it persists across handling
    // of any exceptions.  Otherwise it's possible that the VM never gives up the GIL.
    volatile int gil_divisor = MICROPY_PY_THREAD_GIL_VM_DIVISOR;
    #endif



    MP_STACK_CHECK();

    CTX.self_fun = MP_OBJ_TO_PTR(CTX.self_in);

    DECODE_CODESTATE_SIZE(CTX.self_fun->bytecode, CTX.n_state, CTX.state_size);

    // allocate state for locals and stack
    // mpi_ctx[ctx].code_state == NULL ;
    CTX.code_state = mp_pystack_alloc(sizeof(mp_code_state_t) + CTX.state_size);

    INIT_CODESTATE(CTX.code_state, CTX.self_fun, CTX.n_args, CTX.n_kw, CTX.args); //for __main__ is 0, 0, NULL);

    // execute the byte code with the correct globals context
    mp_globals_set(CTX.self_fun->globals);

    ctx_get_next(CTX_COPY);
    GOSUB(def_mp_execute_bytecode,"func_bc_call");
    CTX.vm_return_kind = CTX.sub_vm_return_kind;

    mp_globals_set(CTX.code_state->old_globals);

    if (CTX.vm_return_kind == MP_VM_RETURN_NORMAL) {
        // return value is in *sp
        RETVAL = *CTX.code_state->sp;
    } else {
        // must be an exception because normal functions can't yield
        assert(CTX.vm_return_kind == MP_VM_RETURN_EXCEPTION);

        // returned exception is in state[0]
        RETVAL = CTX.code_state->state[0];
    }

    mp_pystack_free(CTX.code_state);

    RETURN;
}















#if MICROPY_PY_SYS_EXC_INFO
#define CLEAR_SYS_EXC_INFO() MP_STATE_VM(cur_exception) = NULL;
#else
#define CLEAR_SYS_EXC_INFO()
#endif



#define VM_PUSH(val) *++CTX.sp = (val)
#define VM_POP() (*CTX.sp--)
#define VM_TOP() (*CTX.sp)
#define VM_SET_TOP(val) *CTX.sp = (val)

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






    #define VM_TRACE(ip)

static mp_obj_t obj_shared;
static int GOTO_OUTER_VM_DISPATCH = 1;
static nlr_buf_t nlr;

def_mp_execute_bytecode: {
#if 1
    CTX.vm_return_kind = mp_execute_bytecode(CTX.code_state, MP_OBJ_NULL);
#else
    CTX.inject_exc = MP_OBJ_NULL ;


    #if VM_OPT_COMPUTED_GOTO
        #error "no wasm support"
    #else
        #define VM_ENTRY(op) case op
    #endif


    #if MICROPY_STACKLESS
run_code_state: ;
    #endif

    //if (show_os_loop(-1)) fprintf(stderr,"mpsl:76 VM(%d,%d)run_code_state\n", ctx_current,VM_depth);

    // Pointers which are constant for particular invocation of mp_execute_bytecode()
    //mp_obj_t * /*const*/ fastn;
    //mp_exc_stack_t * /*const*/ exc_stack;


    {
        size_t n_state = mp_decode_uint_value(CTX.code_state->fun_bc->bytecode);
        CTX.fastn = &CTX.code_state->state[n_state - 1];
        CTX.exc_stack = (mp_exc_stack_t*)(CTX.code_state->state + n_state);
    }

    // variables that are visible to the exception handler (declared volatile)
    CTX.exc_sp = MP_TAGPTR_PTR(CTX.code_state->exc_sp); // stack grows up, exc_sp points to top of stack

VM_DISPATCH_loop:

    // loop to execute byte code
    for (;;) {

        if ( CTX.vmloop_state == VM_RESUMING ) {
            //? restore what ?
            clog("mpsl:195 VM(%d)restored", ctx_current);
            //CTX.ip = CTX.code_state->ip;
            //CTX.sp = CTX.code_state->sp;
            //obj_shared = CTX.obj_shared ;
            CTX.vmloop_state = VM_RUNNING;
        }  else {

            if (GOTO_OUTER_VM_DISPATCH) {
                GOTO_OUTER_VM_DISPATCH = 0;
                // local variables that are not visible to the exception handler
                //static const byte *ip;
                CTX.ip = CTX.code_state->ip;
                //static mp_obj_t *sp;
                CTX.sp = CTX.code_state->sp;
            }

            MICROPY_VM_HOOK_INIT
        }

        // If we have exception to inject, now that we finish setting up
        // execution context, raise it. This works as if RAISE_VARARGS
        // bytecode was executed.
        // Injecting exc into yield from generator is a special case,
        // handled by MP_BC_YIELD_FROM itself
        if (CTX.inject_exc != MP_OBJ_NULL && *CTX.ip != MP_BC_YIELD_FROM) {
            mp_obj_t exc = CTX.inject_exc;
            CTX.inject_exc = MP_OBJ_NULL;
            exc = mp_make_raise_obj(exc);
            RAISE(exc);
        }

        // outer exception handling loop
        if (nlr_push(&nlr) != 0) {
            clog("outer exception handling loop");
            #include "vm/stackless_ex.c"
            GOTO_OUTER_VM_DISPATCH = 1;continue;
        }

        VM_TRACE(CTX.ip);
        VM_MARK_EXC_IP_GLOBAL();

        #include "vm/stackless_switch.c"

        if (CTX.switch_break_for) break;

//pending_exception_check:
            MICROPY_VM_HOOK_LOOP

            #if MICROPY_ENABLE_SCHEDULER
            // This is an inlined variant of mp_handle_pending
            if (MP_STATE_VM(sched_state) == MP_SCHED_PENDING) {
                MARK_EXC_IP_SELECTIVE();
                mp_uint_t atomic_state = MICROPY_BEGIN_ATOMIC_SECTION();
                mp_obj_t obj = MP_STATE_VM(mp_pending_exception);
                if (obj != MP_OBJ_NULL) {
                    MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
                    if (!mp_sched_num_pending()) {
                        MP_STATE_VM(sched_state) = MP_SCHED_IDLE;
                    }
                    MICROPY_END_ATOMIC_SECTION(atomic_state);
                    RAISE(obj);
                }
                mp_handle_pending_tail(atomic_state);
            }
            #else
            // This is an inlined variant of mp_handle_pending
            if (MP_STATE_VM(mp_pending_exception) != MP_OBJ_NULL) {
                MARK_EXC_IP_SELECTIVE();
                mp_obj_t obj = MP_STATE_VM(mp_pending_exception);
                MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
                RAISE(obj);
            }
            #endif

            #if MICROPY_PY_THREAD_GIL
                #if MICROPY_PY_THREAD_GIL_VM_DIVISOR
                if (--gil_divisor == 0)
                #endif
                {
                    #if MICROPY_PY_THREAD_GIL_VM_DIVISOR
                    gil_divisor = MICROPY_PY_THREAD_GIL_VM_DIVISOR;
                    #endif
                    #if MICROPY_ENABLE_SCHEDULER
                    // can only switch threads if the scheduler is unlocked
                    if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE)
                    #endif
                    {
                    MP_THREAD_GIL_EXIT();
                    MP_THREAD_GIL_ENTER();
                    }
                }
            #endif

    } // for loop

    CTX.switch_break_for = 0 ;
#endif
    RETURN;
}




























//
