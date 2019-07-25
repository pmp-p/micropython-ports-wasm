#define MICROPY_ENABLE_PYSTACK (1)
#define MICROPY_STACKLESS (1)

#define VM_PUSH(val) *++CTX.sp = (val)
#define VM_POP() (*CTX.sp--)
#define VM_TOP() (*CTX.sp)
#define VM_SET_TOP(val) *CTX.sp = (val)

#if 0
    #define VM_SHOW_TRACE 0
    #define VM_TRACE(ip) if (show_os_loop(-1)) { fprintf(stderr, "sp=%d ", (int)(CTX.sp - &CTX.code_state->state[0] + 1)); \
        mp_bytecode_print2(ip, 1, CTX.code_state->fun_bc->const_table); }
#else
    #define VM_SHOW_TRACE 0
    #define VM_TRACE(ip)
#endif


#if MICROPY_PERSISTENT_CODE
/*
#define VM_DECODE_QSTR \
    CTX.qst = ip[0] | ip[1] << 8; \
    CTX.ip += 2;

#define VM_DECODE_PTR \
    VM_DECODE_UINT; \
    CTX.ptr = (void*)(uintptr_t)CTX.code_state->fun_bc->const_table[unum]

#define VM_DECODE_OBJ \
    VM_DECODE_UINT; \
    mp_obj_t obj = (mp_obj_t)CTX.code_state->fun_bc->const_table[unum]
*/
#else
    #error "VM_DECODE_QSTR/VM_DECODE_PTR/VM_DECODE_OBJ"
#endif


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


goto VM_fun_bc_call;

VM_jump_table_entry: {
    void* jump_entry;
    jump_entry = ENTRY_POINT;
    ENTRY_POINT = JMP_NONE;
    goto *jump_entry;
}

VM_jump_table_exit: {
    void* jump_exit;
    jump_exit = EXIT_POINT ;
    EXIT_POINT = JMP_NONE;
    goto *jump_exit;
}


// Begin : fun_bc_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args)
VM_fun_bc_call:
    clog("mpsl:%d fun_bc_call", ctx_current);

MP_STACK_CHECK();

CTX.self = MP_OBJ_TO_PTR(CTX.self_in);


DECODE_CODESTATE_SIZE(CTX.self->bytecode, CTX.n_state, CTX.state_size);

// allocate state for locals and stack
// mpi_ctx[ctx].code_state == NULL ;
CTX.code_state = NULL;

#if MICROPY_ENABLE_PYSTACK
    CTX.code_state = mp_pystack_alloc(sizeof(mp_code_state_t) + CTX.state_size);
#else
if (CTX.state_size > VM_MAX_STATE_ON_STACK) {
    CTX.code_state = m_new_obj_var_maybe(mp_code_state_t, byte, CTX.state_size);
    #if MICROPY_DEBUG_VM_STACK_OVERFLOW
    if (CTX.code_state != NULL) {
        memset(CTX.code_state->state, 0, CTX.state_size);
    }
    #endif
}

if (CTX.code_state == NULL) {
    CTX.code_state = alloca(sizeof(mp_code_state_t) + CTX.state_size);
    #if MICROPY_DEBUG_VM_STACK_OVERFLOW
        memset(CTX.code_state->state, 0, CTX.state_size);
    #endif
    CTX.state_size = 0; // indicate that we allocated using alloca
}
#endif

INIT_CODESTATE(CTX.code_state, CTX.self, CTX.n_args, CTX.n_kw, CTX.args); //for __main__ is 0, 0, NULL);

// execute the byte code with the correct globals context
mp_globals_set(CTX.self->globals);

// End : fun_bc_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args)
// ================================================================================================
VM_mp_execute_bytecode:
// ================================================================================================
// Begin : mp_execute_bytecode(mp_code_state_t *code_state, volatile mp_obj_t inject_exc)


CTX.inject_exc = MP_OBJ_NULL ;


#define MARK_EXC_IP_SELECTIVE()

// stores ip pointing to last opcode
#define VM_MARK_EXC_IP_GLOBAL() { CTX.code_state->ip = CTX.ip; }



#if VM_OPT_COMPUTED_GOTO
    #include "py/vmVM_ENTRYtable.h"
    #define VM_DISPATCH() do { \
        VM_TRACE(ip); \
        VM_MARK_EXC_IP_GLOBAL(); \
        goto *VM_ENTRY_table[*ip++]; \
    } while (0)
    #define VM_DISPATCH_WITH_PEND_EXC_CHECK() goto pending_exception_check
    #define VM_ENTRY(op) VM_ENTRY_##op
    #define VM_ENTRY_DEFAULT VM_ENTRY_default
#else
    #define VM_DISPATCH() goto VM_DISPATCH_loop
    #define VM_DISPATCH_WITH_PEND_EXC_CHECK() goto pending_exception_check
    #define VM_ENTRY(op) case op
    #define VM_ENTRY_DEFAULT default
#endif

// nlr_raise needs to be implemented as a goto, so that the C compiler's flow analyser
// sees that it's possible for us to jump from the VM_DISPATCH loop to the exception
// handler.  Without this, the code may have a different stack layout in the VM_DISPATCH
// loop and the exception handler, leading to very obscure bugs.

#define RAISE(o) do { nlr_pop(); nlr.ret_val = MP_OBJ_TO_PTR(o); goto exception_handler; } while (0)


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

    #if MICROPY_PY_THREAD_GIL && MICROPY_PY_THREAD_GIL_VM_DIVISOR
    // This needs to be volatile and outside the VM loop so it persists across handling
    // of any exceptions.  Otherwise it's possible that the VM never gives up the GIL.
    volatile int gil_divisor = MICROPY_PY_THREAD_GIL_VM_DIVISOR;
    #endif

goto VM_continue;

VM_resume:
    clog("mpsl:169 VM(%d)resume\n", ctx_current);

VM_continue:
    // outer exception handling loop
    for (;;) {

outer_VM_DISPATCH_loop:
        if (nlr_push(&nlr) == 0) {
            // local variables that are not visible to the exception handler
            //static const byte *ip;
            CTX.ip = CTX.code_state->ip;
            //static mp_obj_t *sp;
            CTX.sp = CTX.code_state->sp;

            static mp_obj_t obj_shared;

if ( CTX.vmloop_state != VM_RESUMING ) {
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

            // loop to execute byte code
            for (;;) {

VM_DISPATCH_loop:
    if ( CTX.vmloop_state == VM_RESUMING ) {
     //? restore what ?
    if (show_os_loop(-1))
        fprintf(stderr,"mpsl:169 VM(%d)restored\n", ctx_current);
            CTX.ip = CTX.code_state->ip;
            CTX.sp = CTX.code_state->sp;
            obj_shared = CTX.obj_shared ;
    }
    CTX.vmloop_state = VM_RUNNING;
#if VM_OPT_COMPUTED_GOTO
                VM_DISPATCH();
#else
    VM_skip:
            VM_TRACE(CTX.ip);
            VM_MARK_EXC_IP_GLOBAL();

    switch (*CTX.ip++) {
#endif

                VM_ENTRY(MP_BC_LOAD_CONST_FALSE):
                    VM_PUSH(mp_const_false);
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_LOAD_CONST_NONE):
                    VM_PUSH(mp_const_none);
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_LOAD_CONST_TRUE):
                    VM_PUSH(mp_const_true);
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_LOAD_CONST_SMALL_INT): {
                    mp_int_t num = 0;
                    if ((CTX.ip[0] & 0x40) != 0) {
                        // Number is negative
                        num--;
                    }
                    do {
                        num = (num << 7) | (*CTX.ip & 0x7f);
                    } while ((*CTX.ip++ & 0x80) != 0);
                    VM_PUSH(MP_OBJ_NEW_SMALL_INT(num));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_LOAD_CONST_STRING): {
                    VM_DECODE_QSTR;
                    VM_PUSH(MP_OBJ_NEW_QSTR(CTX.qst));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_LOAD_CONST_OBJ): {
                    VM_DECODE_OBJ;
                    VM_PUSH(obj);
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_LOAD_NULL):
                    VM_PUSH(MP_OBJ_NULL);
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_LOAD_FAST_N): {
                    VM_DECODE_UINT;
                    obj_shared = CTX.fastn[-unum];
                    load_check:
                    if (obj_shared == MP_OBJ_NULL) {
                        local_name_error: {
                            MARK_EXC_IP_SELECTIVE();
                            mp_obj_t obj = mp_obj_new_exception_msg(&mp_type_NameError, "local variable referenced before assignment");
                            RAISE(obj);
                        }
                    }
                    VM_PUSH(obj_shared);
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_LOAD_DEREF): {
                    VM_DECODE_UINT;
                    obj_shared = mp_obj_cell_get(CTX.fastn[-unum]);
                    goto load_check;
                }

                #if !MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE
                VM_ENTRY(MP_BC_LOAD_NAME): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    VM_PUSH(mp_load_name(CTX.qst));
                    VM_DISPATCH();
                }
                #else
                VM_ENTRY(MP_BC_LOAD_NAME): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_obj_t key = MP_OBJ_NEW_QSTR(CTX.qst);
                    mp_uint_t x = *CTX.ip;
                    if (x < mp_locals_get()->map.alloc && mp_locals_get()->map.table[x].key == key) {
                        VM_PUSH(mp_locals_get()->map.table[x].value);
                    } else {
                        mp_map_elem_t *elem = mp_map_lookup(&mp_locals_get()->map, MP_OBJ_NEW_QSTR(CTX.qst), MP_MAP_LOOKUP);
                        if (elem != NULL) {
                            *(byte*)CTX.ip = (elem - &mp_locals_get()->map.table[0]) & 0xff;
                            VM_PUSH(elem->value);
                        } else {
                            VM_PUSH(mp_load_name(MP_OBJ_QSTR_VALUE(key)));
                        }
                    }
                    CTX.ip++;
                    VM_DISPATCH();
                }
                #endif

                #if !MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE
                VM_ENTRY(MP_BC_LOAD_GLOBAL): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    VM_PUSH(mp_load_global(CTX.qst));
                    VM_DISPATCH();
                }
                #else
                VM_ENTRY(MP_BC_LOAD_GLOBAL): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_obj_t key = MP_OBJ_NEW_QSTR(CTX.qst);
                    mp_uint_t x = *CTX.ip;
                    if (x < mp_globals_get()->map.alloc && mp_globals_get()->map.table[x].key == key) {
                        VM_PUSH(mp_globals_get()->map.table[x].value);
                    } else {
                        mp_map_elem_t *elem = mp_map_lookup(&mp_globals_get()->map, MP_OBJ_NEW_QSTR(CTX.qst), MP_MAP_LOOKUP);
                        if (elem != NULL) {
                            *(byte*)CTX.ip = (elem - &mp_globals_get()->map.table[0]) & 0xff;
                            VM_PUSH(elem->value);
                        } else {
                            VM_PUSH(mp_load_global(MP_OBJ_QSTR_VALUE(key)));
                        }
                    }
                    CTX.ip++;
                    VM_DISPATCH();
                }
                #endif

                #if !MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE
                VM_ENTRY(MP_BC_LOAD_ATTR): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    VM_SET_TOP(mp_load_attr(VM_TOP(), CTX.qst));
                    VM_DISPATCH();
                }
                #else
                VM_ENTRY(MP_BC_LOAD_ATTR): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_obj_t top = VM_TOP();
                    if (mp_obj_is_instance_type(mp_obj_get_type(top))) {
                        mp_obj_instance_t *self = MP_OBJ_TO_PTR(top);
                        mp_uint_t x = *CTX.ip;
                        mp_obj_t key = MP_OBJ_NEW_QSTR(CTX.qst);
                        mp_map_elem_t *elem;
                        if (x < self->members.alloc && self->members.table[x].key == key) {
                            elem = &self->members.table[x];
                        } else {
                            elem = mp_map_lookup(&self->members, key, MP_MAP_LOOKUP);
                            if (elem != NULL) {
                                *(byte*)CTX.ip = elem - &self->members.table[0];
                            } else {
                                goto load_attr_cache_fail;
                            }
                        }
                        VM_SET_TOP(elem->value);
                        CTX.ip++;
                        VM_DISPATCH();
                    }
                load_attr_cache_fail:
                    VM_SET_TOP(mp_load_attr(top, CTX.qst));
                    CTX.ip++;
                    VM_DISPATCH();
                }
                #endif

                VM_ENTRY(MP_BC_LOAD_METHOD): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_load_method(*CTX.sp, CTX.qst, CTX.sp);
                    CTX.sp += 1;
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_LOAD_SUPER_METHOD): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    CTX.sp -= 1;
                    mp_load_super_method(CTX.qst, CTX.sp - 1);
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_LOAD_BUILD_CLASS):
                    MARK_EXC_IP_SELECTIVE();
                    VM_PUSH(mp_load_build_class());
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_LOAD_SUBSCR): {
                    MARK_EXC_IP_SELECTIVE();
                    mp_obj_t index = VM_POP();
                    VM_SET_TOP(mp_obj_subscr(VM_TOP(), index, MP_OBJ_SENTINEL));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_STORE_FAST_N): {
                    VM_DECODE_UINT;
                    CTX.fastn[-unum] = VM_POP();
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_STORE_DEREF): {
                    VM_DECODE_UINT;
                    mp_obj_cell_set(CTX.fastn[-unum], VM_POP());
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_STORE_NAME): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_store_name(CTX.qst, VM_POP());
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_STORE_GLOBAL): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_store_global(CTX.qst, VM_POP());
                    VM_DISPATCH();
                }

                #if !MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE
                VM_ENTRY(MP_BC_STORE_ATTR): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_store_attr(CTX.sp[0], CTX.qst, CTX.sp[-1]);
                    CTX.sp -= 2;
                    VM_DISPATCH();
                }
                #else
                // This caching code works with MICROPY_PY_BUILTINS_PROPERTY and/or
                // MICROPY_PY_DESCRIPTORS enabled because if the attr exists in
                // self->members then it can't be a property or have descriptors.  A
                // consequence of this is that we can't use MP_MAP_LOOKUP_ADD_IF_NOT_FOUND
                // in the fast-path below, because that store could override a property.
                VM_ENTRY(MP_BC_STORE_ATTR): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_obj_t top = VM_TOP();
                    if (mp_obj_is_instance_type(mp_obj_get_type(top)) && CTX.sp[-1] != MP_OBJ_NULL) {
                        mp_obj_instance_t *self = MP_OBJ_TO_PTR(top);
                        mp_uint_t x = *CTX.ip;
                        mp_obj_t key = MP_OBJ_NEW_QSTR(CTX.qst);
                        mp_map_elem_t *elem;
                        if (x < self->members.alloc && self->members.table[x].key == key) {
                            elem = &self->members.table[x];
                        } else {
                            elem = mp_map_lookup(&self->members, key, MP_MAP_LOOKUP);
                            if (elem != NULL) {
                                *(byte*)CTX.ip = elem - &self->members.table[0];
                            } else {
                                goto store_attr_cache_fail;
                            }
                        }
                        elem->value = CTX.sp[-1];
                        CTX.sp -= 2;
                        CTX.ip++;
                        VM_DISPATCH();
                    }
                store_attr_cache_fail:
                    mp_store_attr(CTX.sp[0], CTX.qst, CTX.sp[-1]);
                    CTX.sp -= 2;
                    CTX.ip++;
                    VM_DISPATCH();
                }
                #endif

                VM_ENTRY(MP_BC_STORE_SUBSCR):
                    MARK_EXC_IP_SELECTIVE();
                    mp_obj_subscr(CTX.sp[-1], CTX.sp[0], CTX.sp[-2]);
                    CTX.sp -= 3;
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_DELETE_FAST): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    if (CTX.fastn[-unum] == MP_OBJ_NULL) {
                        goto local_name_error;
                    }
                    CTX.fastn[-unum] = MP_OBJ_NULL;
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_DELETE_DEREF): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    if (mp_obj_cell_get(CTX.fastn[-unum]) == MP_OBJ_NULL) {
                        goto local_name_error;
                    }
                    mp_obj_cell_set(CTX.fastn[-unum], MP_OBJ_NULL);
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_DELETE_NAME): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_delete_name(CTX.qst);
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_DELETE_GLOBAL): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_QSTR;
                    mp_delete_global(CTX.qst);
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_DUP_TOP): {
                    mp_obj_t top = VM_TOP();
                    VM_PUSH(top);
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_DUP_TOP_TWO):
                    CTX.sp += 2;
                    CTX.sp[0] = CTX.sp[-2];
                    CTX.sp[-1] = CTX.sp[-3];
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_POP_TOP):
                    CTX.sp -= 1;
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_ROT_TWO): {
                    mp_obj_t top = CTX.sp[0];
                    CTX.sp[0] = CTX.sp[-1];
                    CTX.sp[-1] = top;
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_ROT_THREE): {
                    mp_obj_t top = CTX.sp[0];
                    CTX.sp[0] = CTX.sp[-1];
                    CTX.sp[-1] = CTX.sp[-2];
                    CTX.sp[-2] = top;
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_JUMP): {
                    VM_DECODE_SLABEL;
                    CTX.ip += CTX.slab;
                    VM_DISPATCH_WITH_PEND_EXC_CHECK();
                }

                VM_ENTRY(MP_BC_POP_JUMP_IF_TRUE): {
                    VM_DECODE_SLABEL;
                    if (mp_obj_is_true(VM_POP())) {
                        CTX.ip += CTX.slab;
                    }
                    VM_DISPATCH_WITH_PEND_EXC_CHECK();
                }

                VM_ENTRY(MP_BC_POP_JUMP_IF_FALSE): {
                    VM_DECODE_SLABEL;
                    if (!mp_obj_is_true(VM_POP())) {
                        CTX.ip += CTX.slab;
                    }
                    VM_DISPATCH_WITH_PEND_EXC_CHECK();
                }

                VM_ENTRY(MP_BC_JUMP_IF_TRUE_OR_POP): {
                    VM_DECODE_SLABEL;
                    if (mp_obj_is_true(VM_TOP())) {
                        CTX.ip += CTX.slab;
                    } else {
                        CTX.sp--;
                    }
                    VM_DISPATCH_WITH_PEND_EXC_CHECK();
                }

                VM_ENTRY(MP_BC_JUMP_IF_FALSE_OR_POP): {
                    VM_DECODE_SLABEL;
                    if (mp_obj_is_true(VM_TOP())) {
                        CTX.sp--;
                    } else {
                        CTX.ip += CTX.slab;
                    }
                    VM_DISPATCH_WITH_PEND_EXC_CHECK();
                }

                VM_ENTRY(MP_BC_SETUP_WITH): {
                    MARK_EXC_IP_SELECTIVE();
                    // stack: (..., ctx_mgr)
                    mp_obj_t obj = VM_TOP();
                    mp_load_method(obj, MP_QSTR___exit__, CTX.sp);
                    mp_load_method(obj, MP_QSTR___enter__, CTX.sp + 2);
                    mp_obj_t ret = mp_call_method_n_kw(0, 0, CTX.sp + 2);
                    CTX.sp += 1;
                    VM_PUSH_EXC_BLOCK(1);
                    VM_PUSH(ret);
                    // stack: (..., __exit__, ctx_mgr, as_value)
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_WITH_CLEANUP): {
                    MARK_EXC_IP_SELECTIVE();
                    // Arriving here, there's "exception control block" on top of stack,
                    // and __exit__ method (with self) underneath it. Bytecode calls __exit__,
                    // and "deletes" it off stack, shifting "exception control block"
                    // to its place.
                    // The bytecode emitter ensures that there is enough space on the Python
                    // value stack to hold the __exit__ method plus an additional 4 entries.
                    if (VM_TOP() == mp_const_none) {
                        // stack: (..., __exit__, ctx_mgr, None)
                        CTX.sp[1] = mp_const_none;
                        CTX.sp[2] = mp_const_none;
                        CTX.sp -= 2;
                        mp_call_method_n_kw(3, 0, CTX.sp);
                        VM_SET_TOP(mp_const_none);
                    } else if (mp_obj_is_small_int(VM_TOP())) {
                        // Getting here there are two distinct cases:
                        //  - unwind return, stack: (..., __exit__, ctx_mgr, ret_val, SMALL_INT(-1))
                        //  - unwind jump, stack:   (..., __exit__, ctx_mgr, dest_ip, SMALL_INT(num_exc))
                        // For both cases we do exactly the same thing.
                        mp_obj_t data = CTX.sp[-1];
                        mp_obj_t cause = CTX.sp[0];
                        CTX.sp[-1] = mp_const_none;
                        CTX.sp[0] = mp_const_none;
                        CTX.sp[1] = mp_const_none;
                        mp_call_method_n_kw(3, 0, CTX.sp - 3);
                        CTX.sp[-3] = data;
                        CTX.sp[-2] = cause;
                        CTX.sp -= 2; // we removed (__exit__, ctx_mgr)
                    } else {
                        assert(mp_obj_is_exception_instance(VM_TOP()));
                        // stack: (..., __exit__, ctx_mgr, exc_instance)
                        // Need to pass (exc_type, exc_instance, None) as arguments to __exit__.
                        CTX.sp[1] = CTX.sp[0];
                        CTX.sp[0] = MP_OBJ_FROM_PTR(mp_obj_get_type(CTX.sp[0]));
                        CTX.sp[2] = mp_const_none;
                        CTX.sp -= 2;
// VM_?
                        mp_obj_t ret_value = mp_call_method_n_kw(3, 0, CTX.sp);
                        if (mp_obj_is_true(ret_value)) {
                            // We need to silence/swallow the exception.  This is done
                            // by popping the exception and the __exit__ handler and
                            // replacing it with None, which signals END_FINALLY to just
                            // execute the finally handler normally.
                            VM_SET_TOP(mp_const_none);
                        } else {
                            // We need to re-raise the exception.  We pop __exit__ handler
                            // by copying the exception instance down to the new top-of-stack.
                            CTX.sp[0] = CTX.sp[3];
                        }
                    }
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_UNWIND_JUMP): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_SLABEL;
                    VM_PUSH((mp_obj_t)(mp_uint_t)(uintptr_t)(CTX.ip + CTX.slab)); // push destination ip for jump
                    VM_PUSH((mp_obj_t)(mp_uint_t)(*CTX.ip)); // push number of exception handlers to unwind (0x80 bit set if we also need to pop stack)
unwind_jump:;
                    mp_uint_t unum = (mp_uint_t)VM_POP(); // get number of exception handlers to unwind
                    while ((unum & 0x7f) > 0) {
                        unum -= 1;
                        assert(CTX.exc_sp >= CTX.exc_stack);
                        // FIXME ()
                        if (MP_TAGPTR_TAG1(CTX.exc_sp->val_sp) && CTX.exc_sp->handler > CTX.ip) {
                            // Getting here the stack looks like:
                            //     (..., X, dest_ip)
                            // where X is pointed to by exc_sp->val_sp and in the case
                            // of a "with" block contains the context manager info.
                            // We're going to run "finally" code as a coroutine
                            // (not calling it recursively). Set up a sentinel
                            // on the stack so it can return back to us when it is
                            // done (when WITH_CLEANUP or END_FINALLY reached).
                            // The sentinel is the number of exception handlers left to
                            // unwind, which is a non-negative integer.
                            VM_PUSH(MP_OBJ_NEW_SMALL_INT(unum));
                            CTX.ip = CTX.exc_sp->handler; // get exception handler byte code address
                            CTX.exc_sp--; // pop exception handler
                            goto VM_DISPATCH_loop; // run the exception handler
                        }
                        VM_POP_EXC_BLOCK();
                    }
                    CTX.ip = (const byte*)MP_OBJ_TO_PTR(VM_POP()); // pop destination ip for jump
                    if (unum != 0) {
                        // pop the exhausted iterator
                        CTX.sp -= MP_OBJ_ITER_BUF_NSLOTS;
                    }
                    VM_DISPATCH_WITH_PEND_EXC_CHECK();
                }

                VM_ENTRY(MP_BC_SETUP_EXCEPT):
                VM_ENTRY(MP_BC_SETUP_FINALLY): {
                    MARK_EXC_IP_SELECTIVE();
                    #if SELECTIVE_EXC_IP
                    VM_PUSH_EXC_BLOCK((code_state->ip[-1] == MP_BC_SETUP_FINALLY) ? 1 : 0);
                    #else
                    VM_PUSH_EXC_BLOCK((CTX.code_state->ip[0] == MP_BC_SETUP_FINALLY) ? 1 : 0);
                    #endif
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_END_FINALLY):
                    MARK_EXC_IP_SELECTIVE();
                    // if VM_TOP is None, just pops it and continues
                    // if VM_TOP is an integer, finishes coroutine and returns control to caller
                    // if VM_TOP is an exception, reraises the exception
                    if (VM_TOP() == mp_const_none) {
                        assert(CTX.exc_sp >= CTX.exc_stack);
                        VM_POP_EXC_BLOCK();
                        CTX.sp--;
                    } else if (mp_obj_is_small_int(VM_TOP())) {
                        // We finished "finally" coroutine and now VM_DISPATCH back
                        // to our caller, based on TOS value
                        mp_int_t cause = MP_OBJ_SMALL_INT_VALUE(VM_POP());
                        if (cause < 0) {
                            // A negative cause indicates unwind return
                            goto unwind_return;
                        } else {
                            // Otherwise it's an unwind jump and we must push as a raw
                            // number the number of exception handlers to unwind
                            VM_PUSH((mp_obj_t)cause);
                            goto unwind_jump;
                        }
                    } else {
                        assert(mp_obj_is_exception_instance(VM_TOP()));
                        RAISE(VM_TOP());
                    }
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_GET_ITER):
                    MARK_EXC_IP_SELECTIVE();
                    VM_SET_TOP(mp_getiter(VM_TOP(), NULL));
                    VM_DISPATCH();

                // An iterator for a for-loop takes MP_OBJ_ITER_BUF_NSLOTS slots on
                // the Python value stack.  These slots are either used to store the
                // iterator object itself, or the first slot is MP_OBJ_NULL and
                // the second slot holds a reference to the iterator object.
                VM_ENTRY(MP_BC_GET_ITER_STACK): {
                    MARK_EXC_IP_SELECTIVE();
                    mp_obj_t obj = VM_TOP();
                    mp_obj_iter_buf_t *iter_buf = (mp_obj_iter_buf_t*)CTX.sp;
                    CTX.sp += MP_OBJ_ITER_BUF_NSLOTS - 1;
                    obj = mp_getiter(obj, iter_buf);
                    if (obj != MP_OBJ_FROM_PTR(iter_buf)) {
                        // Iterator didn't use the stack so indicate that with MP_OBJ_NULL.
                        CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 1] = MP_OBJ_NULL;
                        CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 2] = obj;
                    }
                    VM_DISPATCH();
                }

                #include "vm/bc_for_iter.c"

                VM_ENTRY(MP_BC_POP_EXCEPT_JUMP): {
                    assert(CTX.exc_sp >= CTX.exc_stack);
                    VM_POP_EXC_BLOCK();
                    VM_DECODE_ULABEL;
                    CTX.ip += CTX.ulab;
                    VM_DISPATCH_WITH_PEND_EXC_CHECK();
                }

                VM_ENTRY(MP_BC_BUILD_TUPLE): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    CTX.sp -= unum - 1;
                    VM_SET_TOP(mp_obj_new_tuple(unum, CTX.sp));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_BUILD_LIST): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    CTX.sp -= unum - 1;
                    VM_SET_TOP(mp_obj_new_list(unum, CTX.sp));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_BUILD_MAP): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    VM_PUSH(mp_obj_new_dict(unum));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_STORE_MAP):
                    MARK_EXC_IP_SELECTIVE();
                    CTX.sp -= 2;
                    mp_obj_dict_store(CTX.sp[0], CTX.sp[2], CTX.sp[1]);
                    VM_DISPATCH();

#if MICROPY_PY_BUILTINS_SET
                VM_ENTRY(MP_BC_BUILD_SET): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    CTX.sp -= unum - 1;
                    VM_SET_TOP(mp_obj_new_set(unum, CTX.sp));
                    VM_DISPATCH();
                }
#endif

#if MICROPY_PY_BUILTINS_SLICE
                VM_ENTRY(MP_BC_BUILD_SLICE): {
                    MARK_EXC_IP_SELECTIVE();
                    mp_obj_t step = mp_const_none;
                    if (*CTX.ip++ == 3) {
                        // 3-argument slice includes step
                        step = VM_POP();
                    }
                    mp_obj_t stop = VM_POP();
                    mp_obj_t start = VM_TOP();
                    VM_SET_TOP(mp_obj_new_slice(start, stop, step));
                    VM_DISPATCH();
                }
#endif

                VM_ENTRY(MP_BC_STORE_COMP): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    mp_obj_t obj = CTX.sp[-(unum >> 2)];
                    if ((unum & 3) == 0) {
                        mp_obj_list_append(obj, CTX.sp[0]);
                        CTX.sp--;
                    } else if (!MICROPY_PY_BUILTINS_SET || (unum & 3) == 1) {
                        mp_obj_dict_store(obj, CTX.sp[0], CTX.sp[-1]);
                        CTX.sp -= 2;
                    #if MICROPY_PY_BUILTINS_SET
                    } else {
                        mp_obj_set_store(obj, CTX.sp[0]);
                        CTX.sp--;
                    #endif
                    }
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_UNPACK_SEQUENCE): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    mp_unpack_sequence(CTX.sp[0], unum, CTX.sp);
                    CTX.sp += unum - 1;
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_UNPACK_EX): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    mp_unpack_ex(CTX.sp[0], unum, CTX.sp);
                    CTX.sp += (unum & 0xff) + ((unum >> 8) & 0xff);
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_MAKE_FUNCTION): {
                    VM_DECODE_PTR;
                    VM_PUSH(mp_make_function_from_raw_code(CTX.ptr, MP_OBJ_NULL, MP_OBJ_NULL));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_MAKE_FUNCTION_DEFARGS): {
                    VM_DECODE_PTR;
                    // Stack layout: def_tuple def_dict <- TOS
                    mp_obj_t def_dict = VM_POP();
                    VM_SET_TOP(mp_make_function_from_raw_code(CTX.ptr, VM_TOP(), def_dict));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_MAKE_CLOSURE): {
                    VM_DECODE_PTR;
                    size_t n_closed_over = *CTX.ip++;
                    // Stack layout: closed_overs <- TOS
                    CTX.sp -= n_closed_over - 1;
                    VM_SET_TOP(mp_make_closure_from_raw_code(CTX.ptr, n_closed_over, CTX.sp));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_MAKE_CLOSURE_DEFARGS): {
                    VM_DECODE_PTR;
                    size_t n_closed_over = *CTX.ip++;
                    // Stack layout: def_tuple def_dict closed_overs <- TOS
                    CTX.sp -= 2 + n_closed_over - 1;
                    VM_SET_TOP(mp_make_closure_from_raw_code(CTX.ptr, 0x100 | n_closed_over, CTX.sp));
                    VM_DISPATCH();
                }

                #include "vm/bc_call_function.c"

                #include "vm/bc_call_function_var_kw.c"

                #include "vm/bc_call_method.c"


                VM_ENTRY(MP_BC_CALL_METHOD_VAR_KW): {
                    MARK_EXC_IP_SELECTIVE();
                    VM_DECODE_UINT;
                    // unum & 0xff == n_positional
                    // (unum >> 8) & 0xff == n_keyword
                    // We have following stack layout here:
                    // fun self arg0 arg1 ... kw0 val0 kw1 val1 ... seq dict <- TOS
                    CTX.sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe) + 3;
                    #if MICROPY_STACKLESS
                    if (mp_obj_get_type(*CTX.sp) == &mp_type_fun_bc) {
                        CTX.code_state->ip = CTX.ip;
                        CTX.code_state->sp = CTX.sp;
                        CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);

                        mp_call_args_t out_args;
                        mp_call_prepare_args_n_kw_var(true, unum, CTX.sp, &out_args);

                        mp_code_state_t *new_state = mp_obj_fun_bc_prepare_codestate(out_args.fun,
                            out_args.n_args, out_args.n_kw, out_args.args);
                        #if !MICROPY_ENABLE_PYSTACK
                        // Freeing args at this point does not follow a LIFO order so only do it if
                        // pystack is not enabled.  For pystack, they are freed when code_state is.
                        mp_nonlocal_free(out_args.args, out_args.n_alloc * sizeof(mp_obj_t));
                        #endif
                        #if !MICROPY_ENABLE_PYSTACK
                        if (new_state == NULL) {
                            // Couldn't allocate codestate on heap: in the strict case raise
                            // an exception, otherwise just fall through to stack allocation.
                            #if MICROPY_STACKLESS_STRICT
                            goto deep_recursion_error;
                            #endif
                        } else
                        #endif
                        {
                            new_state->prev = CTX.code_state;
                            CTX.code_state = new_state;
                            nlr_pop();
                            goto run_code_state;
                        }
                    }
                    #endif
                    VM_SET_TOP(mp_call_method_n_kw_var(true, unum, CTX.sp));
                    VM_DISPATCH();
                }

                VM_ENTRY(MP_BC_RETURN_VALUE):
                    MARK_EXC_IP_SELECTIVE();
unwind_return:
                    // Search for and execute finally handlers that aren't already active
                    while (CTX.exc_sp >= CTX.exc_stack) {
                        if (MP_TAGPTR_TAG1(CTX.exc_sp->val_sp) && CTX.exc_sp->handler > CTX.ip) {
                            // Found a finally handler that isn't active.
                            // Getting here the stack looks like:
                            //     (..., X, [iter0, iter1, ...,] ret_val)
                            // where X is pointed to by exc_sp->val_sp and in the case
                            // of a "with" block contains the context manager info.
                            // There may be 0 or more for-iterators between X and the
                            // return value, and these must be removed before control can
                            // pass to the finally code.  We simply copy the ret_value down
                            // over these iterators, if they exist.  If they don't then the
                            // following is a null operation.
                            mp_obj_t *finally_sp = MP_TAGPTR_PTR(CTX.exc_sp->val_sp);
                            finally_sp[1] = CTX.sp[0];
                            CTX.sp = &finally_sp[1];
                            // We're going to run "finally" code as a coroutine
                            // (not calling it recursively). Set up a sentinel
                            // on a stack so it can return back to us when it is
                            // done (when WITH_CLEANUP or END_FINALLY reached).
                            VM_PUSH(MP_OBJ_NEW_SMALL_INT(-1));
                            CTX.ip = CTX.exc_sp->handler;
                            VM_POP_EXC_BLOCK();
                            goto VM_DISPATCH_loop;
                        }
                        VM_POP_EXC_BLOCK();
                    }
                    nlr_pop();
                    CTX.code_state->sp = CTX.sp;
                    assert(CTX.exc_sp == CTX.exc_stack - 1);
                    MICROPY_VM_HOOK_RETURN
                    #if MICROPY_STACKLESS
                    if (CTX.code_state->prev != NULL) {
                        mp_obj_t res = *CTX.sp;
                        mp_globals_set(CTX.code_state->old_globals);
                        mp_code_state_t *new_code_state = CTX.code_state->prev;
                        #if MICROPY_ENABLE_PYSTACK
                        // Free code_state, and args allocated by mp_call_prepare_args_n_kw_var
                        // (The latter is implicitly freed when using pystack due to its LIFO nature.)
                        // The sizeof in the following statement does not include the size of the variable
                        // part of the struct.  This arg is anyway not used if pystack is enabled.
                        mp_nonlocal_free(CTX.code_state, sizeof(mp_code_state_t));
                        #endif
                        CTX.code_state = new_code_state;
                        *CTX.code_state->sp = res;
                        goto run_code_state;
                    }
                    #endif
                    //return MP_VM_RETURN_NORMAL;
                    CTX.vm_return_kind = MP_VM_RETURN_NORMAL;
                    goto VM_mp_execute_bytecode_return;

                VM_ENTRY(MP_BC_RAISE_VARARGS): {
if (show_os_loop(-1))
    fprintf(stderr,"mpsl:1139 raise\n");
                    MARK_EXC_IP_SELECTIVE();
                    mp_uint_t unum = *CTX.ip;
                    mp_obj_t obj;
                    if (unum == 2) {
                        mp_warning(NULL, "exception chaining not supported");
                        // ignore (pop) "from" argument
                        CTX.sp--;
                    }
                    if (unum == 0) {
                        // search for the inner-most previous exception, to reraise it
                        obj = MP_OBJ_NULL;
                        for (mp_exc_stack_t *e = CTX.exc_sp; e >= CTX.exc_stack; e--) {
                            if (e->prev_exc != NULL) {
                                obj = MP_OBJ_FROM_PTR(e->prev_exc);
                                break;
                            }
                        }
                        if (obj == MP_OBJ_NULL) {
                            obj = mp_obj_new_exception_msg(&mp_type_RuntimeError, "no active exception to reraise");
                            RAISE(obj);
                        }
                    } else {
                        obj = VM_TOP();
                    }
                    obj = mp_make_raise_obj(obj);
                    RAISE(obj);
                }

                VM_ENTRY(MP_BC_YIELD_VALUE):
yield:
                    nlr_pop();
                    CTX.code_state->ip = CTX.ip;
                    CTX.code_state->sp = CTX.sp;
                    CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);
                    CTX.vm_return_kind =  MP_VM_RETURN_YIELD;
                    goto VM_mp_execute_bytecode_return;

                VM_ENTRY(MP_BC_YIELD_FROM): {
                    MARK_EXC_IP_SELECTIVE();
//#define EXC_MATCH(exc, type) mp_obj_is_type(exc, type)
#define EXC_MATCH(exc, type) mp_obj_exception_match(exc, type)
#define GENERATOR_EXIT_IF_NEEDED(t) if (t != MP_OBJ_NULL && EXC_MATCH(t, MP_OBJ_FROM_PTR(&mp_type_GeneratorExit))) { mp_obj_t raise_t = mp_make_raise_obj(t); RAISE(raise_t); }
                    mp_vm_return_kind_t ret_kind;
                    mp_obj_t send_value = VM_POP();
                    mp_obj_t t_exc = MP_OBJ_NULL;
                    mp_obj_t ret_value;
                    CTX.code_state->sp = CTX.sp; // Save sp because it's needed if mp_resume raises StopIteration
                    if (CTX.inject_exc != MP_OBJ_NULL) {
                        t_exc = CTX.inject_exc;
                        CTX.inject_exc = MP_OBJ_NULL;
                        if (show_os_loop(-1))
                            fprintf(stderr,"mpsl:1052 resume + exc\n");
                        ret_kind = mp_resume(VM_TOP(), MP_OBJ_NULL, t_exc, &ret_value);
                    } else {
                        if (show_os_loop(-1))
                            fprintf(stderr,"mpsl:1056 resume\n");
                        ret_kind = mp_resume(VM_TOP(), send_value, MP_OBJ_NULL, &ret_value);
                    }

                    if (ret_kind == MP_VM_RETURN_YIELD) {
                        fprintf(stderr,"mpsl:1061 yield from\n");
                        CTX.ip--;
                        VM_PUSH(ret_value);
                        goto yield;
                    } else if (ret_kind == MP_VM_RETURN_NORMAL) {
                        // Pop exhausted gen
                        CTX.sp--;
                        if (ret_value == MP_OBJ_STOP_ITERATION) {
                            // Optimize StopIteration
                            // TODO: get StopIteration's value
                            VM_PUSH(mp_const_none);
                        } else {
                            VM_PUSH(ret_value);
                        }

                        // If we injected GeneratorExit downstream, then even
                        // if it was swallowed, we re-raise GeneratorExit
                        GENERATOR_EXIT_IF_NEEDED(t_exc);
                        VM_DISPATCH();
                    } else {
                        assert(ret_kind == MP_VM_RETURN_EXCEPTION);
                        // Pop exhausted gen
                        CTX.sp--;
                        if (EXC_MATCH(ret_value, MP_OBJ_FROM_PTR(&mp_type_StopIteration))) {
                            VM_PUSH(mp_obj_exception_get_value(ret_value));
                            // If we injected GeneratorExit downstream, then even
                            // if it was swallowed, we re-raise GeneratorExit
                            GENERATOR_EXIT_IF_NEEDED(t_exc);
                            VM_DISPATCH();
                        } else {
                            RAISE(ret_value);
                        }
                    }
                }

                #include "vm/bc_import.c"

#if VM_OPT_COMPUTED_GOTO
                VM_ENTRY(MP_BC_LOAD_CONST_SMALL_INT_MULTI):
                    VM_PUSH(MP_OBJ_NEW_SMALL_INT((mp_int_t)ip[-1] - MP_BC_LOAD_CONST_SMALL_INT_MULTI - 16));
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_LOAD_FAST_MULTI):
                    obj_shared = fastn[MP_BC_LOAD_FAST_MULTI - (mp_int_t)ip[-1]];
                    goto load_check;

                VM_ENTRY(MP_BC_STORE_FAST_MULTI):
                    fastn[MP_BC_STORE_FAST_MULTI - (mp_int_t)ip[-1]] = VM_POP();
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_UNARY_OP_MULTI):
                    MARK_EXC_IP_SELECTIVE();
                    VM_SET_TOP(mp_unary_op(CTX.ip[-1] - MP_BC_UNARY_OP_MULTI, VM_TOP()));
                    VM_DISPATCH();

                VM_ENTRY(MP_BC_BINARY_OP_MULTI): {
                    MARK_EXC_IP_SELECTIVE();
                    mp_obj_t rhs = VM_POP();
                    mp_obj_t lhs = VM_TOP();
                    VM_SET_TOP(mp_binary_op(CTX.ip[-1] - MP_BC_BINARY_OP_MULTI, lhs, rhs));
                    VM_DISPATCH();
                }

                VM_ENTRY_DEFAULT:
                    MARK_EXC_IP_SELECTIVE();
#else
                VM_ENTRY_DEFAULT:
                    if (CTX.ip[-1] < MP_BC_LOAD_CONST_SMALL_INT_MULTI + 64) {
                        VM_PUSH(MP_OBJ_NEW_SMALL_INT((mp_int_t)CTX.ip[-1] - MP_BC_LOAD_CONST_SMALL_INT_MULTI - 16));
                        VM_DISPATCH();
                    } else if (CTX.ip[-1] < MP_BC_LOAD_FAST_MULTI + 16) {
                        obj_shared = CTX.fastn[MP_BC_LOAD_FAST_MULTI - (mp_int_t)CTX.ip[-1]];
                        goto load_check;
                    } else if (CTX.ip[-1] < MP_BC_STORE_FAST_MULTI + 16) {
                        CTX.fastn[MP_BC_STORE_FAST_MULTI - (mp_int_t)CTX.ip[-1]] = VM_POP();
                        VM_DISPATCH();
                    } else if (CTX.ip[-1] < MP_BC_UNARY_OP_MULTI + MP_UNARY_OP_NUM_BYTECODE) {
                        VM_SET_TOP(mp_unary_op(CTX.ip[-1] - MP_BC_UNARY_OP_MULTI, VM_TOP()));
                        VM_DISPATCH();
                    } else if (CTX.ip[-1] < MP_BC_BINARY_OP_MULTI + MP_BINARY_OP_NUM_BYTECODE) {
                        mp_obj_t rhs = VM_POP();
                        mp_obj_t lhs = VM_TOP();
                        VM_SET_TOP(mp_binary_op(CTX.ip[-1] - MP_BC_BINARY_OP_MULTI, lhs, rhs));
                        VM_DISPATCH();
                    } else
#endif
                {
                    mp_obj_t obj = mp_obj_new_exception_msg(&mp_type_NotImplementedError, "byte code not implemented");
                    nlr_pop();
                    CTX.code_state->state[0] = obj;
                    CTX.vm_return_kind =  MP_VM_RETURN_EXCEPTION;
                    goto VM_mp_execute_bytecode_return;
                }

#if !VM_OPT_COMPUTED_GOTO
                } // switch
#endif

pending_exception_check:
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

        } else {
exception_handler:
            // exception occurred

            if (show_os_loop(0)) fprintf(stderr,"mpsl:1270 except\n");
            #if MICROPY_PY_SYS_EXC_INFO
            MP_STATE_VM(cur_exception) = nlr.ret_val;
            #endif

            #if SELECTIVE_EXC_IP
            // with selective ip, we store the ip 1 byte past the opcode, so move CTX.ptr back
            code_state->ip -= 1;
            #endif

            if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(((mp_obj_base_t*)nlr.ret_val)->type), MP_OBJ_FROM_PTR(&mp_type_StopIteration))) {
                if (CTX.code_state->ip) {
                    // check if it's a StopIteration within a for block
                    if (*CTX.code_state->ip == MP_BC_FOR_ITER) {
                        const byte *ip = CTX.code_state->ip + 1;
                        VM_DECODE_ULABEL; // the jump offset if iteration finishes; for labels are always forward
                        CTX.code_state->ip = ip + CTX.ulab; // jump to after for-block
                        CTX.code_state->sp -= MP_OBJ_ITER_BUF_NSLOTS; // pop the exhausted iterator
                        goto outer_VM_DISPATCH_loop; // continue with VM_DISPATCH loop

                    } else if (*CTX.code_state->ip == MP_BC_YIELD_FROM) {
                        // StopIteration inside yield from call means return a value of
                        // yield from, so inject exception's value as yield from's result
                        // (Instead of stack pop then push we just replace exhausted gen with value)
                        *CTX.code_state->sp = mp_obj_exception_get_value(MP_OBJ_FROM_PTR(nlr.ret_val));
                        CTX.code_state->ip++; // yield from is over, move to next instruction
                        goto outer_VM_DISPATCH_loop; // continue with VM_DISPATCH loop
                    }
                }
            }



#if MICROPY_STACKLESS
unwind_loop:
#endif
            // set file and line number that the exception occurred at
            // TODO: don't set traceback for exceptions re-raised by END_FINALLY.
            // But consider how to handle nested exceptions.
            if (nlr.ret_val != &mp_const_GeneratorExit_obj) {
                const byte *ip = CTX.code_state->fun_bc->bytecode;
                ip = mp_decode_uint_skip(ip); // skip n_state
                ip = mp_decode_uint_skip(ip); // skip n_exc_stack
                ip++; // skip scope_params
                ip++; // skip n_pos_args
                ip++; // skip n_kwonly_args
                ip++; // skip n_def_pos_args
                size_t bc = CTX.code_state->ip - ip;
                size_t code_info_size = mp_decode_uint_value(ip);
                ip = mp_decode_uint_skip(ip); // skip code_info_size
                bc -= code_info_size;
                #if MICROPY_PERSISTENT_CODE
                qstr block_name = ip[0] | (ip[1] << 8);
                qstr source_file = ip[2] | (ip[3] << 8);
                ip += 4;
                #else
                qstr block_name = mp_decode_uint_value(ip);
                ip = mp_decode_uint_skip(ip);
                qstr source_file = mp_decode_uint_value(ip);
                ip = mp_decode_uint_skip(ip);
                #endif
                size_t source_line = 1;
                size_t c;
                while ((c = *ip)) {
                    size_t b, l;
                    if ((c & 0x80) == 0) {
                        // 0b0LLBBBBB encoding
                        b = c & 0x1f;
                        l = c >> 5;
                        ip += 1;
                    } else {
                        // 0b1LLLBBBB 0bLLLLLLLL encoding (l's LSB in second byte)
                        b = c & 0xf;
                        l = ((c << 4) & 0x700) | ip[1];
                        ip += 2;
                    }
                    if (bc >= b) {
                        bc -= b;
                        source_line += l;
                    } else {
                        // found source line corresponding to bytecode offset
                        break;
                    }
                }
                mp_obj_exception_add_traceback(MP_OBJ_FROM_PTR(nlr.ret_val), source_file, source_line, block_name);
            }

            while ((CTX.exc_sp >= CTX.exc_stack) && (CTX.exc_sp->handler <= CTX.code_state->ip)) {

                // nested exception

                assert(CTX.exc_sp >= CTX.exc_stack);

                // TODO make a proper message for nested exception
                // at the moment we are just raising the very last exception (the one that caused the nested exception)

                // move up to previous exception handler
                VM_POP_EXC_BLOCK();
            }

            if (CTX.exc_sp >= CTX.exc_stack) {
                // catch exception and pass to byte code
                CTX.code_state->ip = CTX.exc_sp->handler;
                mp_obj_t *sp = MP_TAGPTR_PTR(CTX.exc_sp->val_sp);
                // save this exception in the stack so it can be used in a reraise, if needed
                CTX.exc_sp->prev_exc = nlr.ret_val;
                // push exception object so it can be handled by bytecode
                VM_PUSH(MP_OBJ_FROM_PTR(nlr.ret_val));
                CTX.code_state->sp = sp;

            #if MICROPY_STACKLESS
            } else if (CTX.code_state->prev != NULL) {
                mp_globals_set(CTX.code_state->old_globals);
                mp_code_state_t *new_code_state = CTX.code_state->prev;
                #if MICROPY_ENABLE_PYSTACK
                // Free code_state, and args allocated by mp_call_prepare_args_n_kw_var
                // (The latter is implicitly freed when using pystack due to its LIFO nature.)
                // The sizeof in the following statement does not include the size of the variable
                // part of the struct.  This arg is anyway not used if pystack is enabled.
                mp_nonlocal_free(CTX.code_state, sizeof(mp_code_state_t));
                #endif
                CTX.code_state = new_code_state;
                size_t n_state = mp_decode_uint_value(CTX.code_state->fun_bc->bytecode);
                CTX.fastn = &CTX.code_state->state[n_state - 1];
                CTX.exc_stack = (mp_exc_stack_t*)(CTX.code_state->state + n_state);
                // variables that are visible to the exception handler (declared volatile)
                CTX.exc_sp = MP_TAGPTR_PTR(CTX.code_state->exc_sp); // stack grows up, exc_sp points to top of stack
                goto unwind_loop;

            #endif
            } else {
                // propagate exception to higher level
                // Note: ip and sp don't have usable values at this point
                CTX.code_state->state[0] = MP_OBJ_FROM_PTR(nlr.ret_val); // put exception here because sp is invalid
                CTX.vm_return_kind = MP_VM_RETURN_EXCEPTION;
                goto VM_mp_execute_bytecode_return;
            }
        }
    }

//===================================================================================================
//===================================================================================================
//===================================================================================================

// End : mp_execute_bytecode(mp_code_state_t *code_state, volatile mp_obj_t inject_exc)
VM_mp_execute_bytecode_return:
    if ( ENTRY_POINT != &&VM_fun_bc_call) {
        if (exit_point[CTX.pointer] != JMP_NONE) {
            clog("mpsl:1475 VM[%d]_mp_execute_bytecode_return JUMP %d\n", ctx_current, CTX.pointer);
            goto VM_jump_table_exit;
        } else
            clog("mpsl:1376 VM[%d]_mp_execute_bytecode_return no jump\n", ctx_current);
    }
//===================================================================================================
//===================================================================================================
//===================================================================================================



mp_globals_set(CTX.code_state->old_globals);

//mp_obj_t result;
if (CTX.vm_return_kind == MP_VM_RETURN_NORMAL) {
    // return value is in *sp
    CTX.return_value = *CTX.code_state->sp;
} else {
    if (CTX.vm_return_kind == MP_VM_RETURN_YIELD) {
            clog("mpsl:1392 CTX YIELD\n");
    } else {
        // must be an exception because normal functions can't yield
        assert(CTX.vm_return_kind == MP_VM_RETURN_EXCEPTION);
        // returned exception is in state[0]
        CTX.return_value = CTX.code_state->state[0];
    }
}

#if MICROPY_ENABLE_PYSTACK
mp_pystack_free(CTX.code_state);
#else
// free the state if it was allocated on the heap
if (CTX.state_size != 0) {
    m_del_var(mp_code_state_t, byte, CTX.state_size, CTX.code_state);
}
#endif

if (CTX.vm_return_kind == MP_VM_RETURN_NORMAL) {
    goto VM_fun_bc_call_return;
} else { // MP_VM_RETURN_EXCEPTION
    nlr_raise(CTX.return_value);
}

// End : fun_bc_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args)

VM_fun_bc_call_return:
    clog("mpsl:1419 VM[%d]_fun_bc_call_return\n", ctx_current);
    if ( EXIT_POINT != JMP_NONE )
        goto VM_jump_table_exit;

