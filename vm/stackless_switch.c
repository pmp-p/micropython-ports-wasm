#define LOCAL_NAME_ERROR() {\
 MARK_EXC_IP_SELECTIVE();\
 mp_obj_t obj = mp_obj_new_exception_msg(&mp_type_NameError, "local variable referenced before assignment");\
 RAISE(obj);\
}


#define LOAD_CHECK() {\
    if (obj_shared == MP_OBJ_NULL)\
        LOCAL_NAME_ERROR();\
}



    switch (*CTX.ip++) {


        VM_ENTRY(MP_BC_LOAD_CONST_FALSE): {
            VM_PUSH(mp_const_false);
            continue;
        }

        VM_ENTRY(MP_BC_LOAD_CONST_NONE): {
            VM_PUSH(mp_const_none);
            continue;
        }

        VM_ENTRY(MP_BC_LOAD_CONST_TRUE): {
            VM_PUSH(mp_const_true);
            continue;
        }

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
            continue;
        }

        VM_ENTRY(MP_BC_LOAD_CONST_STRING): {
            VM_DECODE_QSTR;
            VM_PUSH(MP_OBJ_NEW_QSTR(CTX.qst));
            continue;
        }

        VM_ENTRY(MP_BC_LOAD_CONST_OBJ): {
            VM_DECODE_OBJ;
            VM_PUSH(obj);
            continue;
        }

        VM_ENTRY(MP_BC_LOAD_NULL): {
            VM_PUSH(MP_OBJ_NULL);
            continue;
        }
/*
        VM_ENTRY(MP_BC_LOAD_FAST_N): {
            VM_DECODE_UINT;
            obj_shared = CTX.fastn[-unum];
load_check:
            if (obj_shared == MP_OBJ_NULL) {

local_name_error:
                {
                    MARK_EXC_IP_SELECTIVE();
                    mp_obj_t obj = mp_obj_new_exception_msg(&mp_type_NameError, "local variable referenced before assignment");
                    RAISE(obj);
                }
            }
            VM_PUSH(obj_shared);
            continue;
        }
*/
        VM_ENTRY(MP_BC_LOAD_FAST_N): {
            VM_DECODE_UINT;
            obj_shared = CTX.fastn[-unum];
            LOAD_CHECK();
            VM_PUSH(obj_shared);
            continue;
        }

        VM_ENTRY(MP_BC_LOAD_DEREF): {
            VM_DECODE_UINT;
            obj_shared = mp_obj_cell_get(CTX.fastn[-unum]);
            LOAD_CHECK();
            VM_PUSH(obj_shared);
            continue;
        }

        #if !MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE
        VM_ENTRY(MP_BC_LOAD_NAME): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            VM_PUSH(mp_load_name(CTX.qst));
            continue;
        }

        VM_ENTRY(MP_BC_LOAD_GLOBAL): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            VM_PUSH(mp_load_global(CTX.qst));
            continue;
        }
        VM_ENTRY(MP_BC_LOAD_ATTR): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            VM_SET_TOP(mp_load_attr(VM_TOP(), CTX.qst));
            continue;
        }

        VM_ENTRY(MP_BC_STORE_ATTR): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            mp_store_attr(CTX.sp[0], CTX.qst, CTX.sp[-1]);
            CTX.sp -= 2;
            continue;
        }

        #else
        #include "stackless_bclookup.c"
        #endif

        VM_ENTRY(MP_BC_LOAD_METHOD): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            mp_load_method(*CTX.sp, CTX.qst, CTX.sp);
            CTX.sp += 1;
            continue;
        }

        VM_ENTRY(MP_BC_LOAD_SUPER_METHOD): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            CTX.sp -= 1;
            mp_load_super_method(CTX.qst, CTX.sp - 1);
            continue;
        }

        VM_ENTRY(MP_BC_LOAD_BUILD_CLASS):
            MARK_EXC_IP_SELECTIVE();
            VM_PUSH(mp_load_build_class());
            continue;

        VM_ENTRY(MP_BC_LOAD_SUBSCR): {
            MARK_EXC_IP_SELECTIVE();
            mp_obj_t index = VM_POP();
            VM_SET_TOP(mp_obj_subscr(VM_TOP(), index, MP_OBJ_SENTINEL));
            continue;
        }

        VM_ENTRY(MP_BC_STORE_FAST_N): {
            VM_DECODE_UINT;
            CTX.fastn[-unum] = VM_POP();
            continue;
        }

        VM_ENTRY(MP_BC_STORE_DEREF): {
            VM_DECODE_UINT;
            mp_obj_cell_set(CTX.fastn[-unum], VM_POP());
            continue;
        }

        VM_ENTRY(MP_BC_STORE_NAME): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            mp_store_name(CTX.qst, VM_POP());
            continue;
        }

        VM_ENTRY(MP_BC_STORE_GLOBAL): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            mp_store_global(CTX.qst, VM_POP());
            continue;
        }

        VM_ENTRY(MP_BC_STORE_SUBSCR): {
            MARK_EXC_IP_SELECTIVE();
            mp_obj_subscr(CTX.sp[-1], CTX.sp[0], CTX.sp[-2]);
            CTX.sp -= 3;
            continue;
        }

        VM_ENTRY(MP_BC_DELETE_FAST): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_UINT;
            if (CTX.fastn[-unum] == MP_OBJ_NULL) {
                //goto local_name_error;
                LOCAL_NAME_ERROR();
            }
            CTX.fastn[-unum] = MP_OBJ_NULL;
            continue;
        }

        VM_ENTRY(MP_BC_DELETE_DEREF): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_UINT;
            if (mp_obj_cell_get(CTX.fastn[-unum]) == MP_OBJ_NULL) {
                //goto local_name_error;
                LOCAL_NAME_ERROR();

            }
            mp_obj_cell_set(CTX.fastn[-unum], MP_OBJ_NULL);
            continue;
        }

        VM_ENTRY(MP_BC_DELETE_NAME): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            mp_delete_name(CTX.qst);
            continue;
        }

        VM_ENTRY(MP_BC_DELETE_GLOBAL): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            mp_delete_global(CTX.qst);
            continue;
        }

        VM_ENTRY(MP_BC_DUP_TOP): {
            mp_obj_t top = VM_TOP();
            VM_PUSH(top);
            continue;
        }

        VM_ENTRY(MP_BC_DUP_TOP_TWO):
            CTX.sp += 2;
            CTX.sp[0] = CTX.sp[-2];
            CTX.sp[-1] = CTX.sp[-3];
            continue;

        VM_ENTRY(MP_BC_POP_TOP): {
            CTX.sp -= 1;
            continue;
        }

        VM_ENTRY(MP_BC_ROT_TWO): {
            mp_obj_t top = CTX.sp[0];
            CTX.sp[0] = CTX.sp[-1];
            CTX.sp[-1] = top;
            continue;
        }

        VM_ENTRY(MP_BC_ROT_THREE): {
            mp_obj_t top = CTX.sp[0];
            CTX.sp[0] = CTX.sp[-1];
            CTX.sp[-1] = CTX.sp[-2];
            CTX.sp[-2] = top;
            continue;
        }

        VM_ENTRY(MP_BC_JUMP): {
            VM_DECODE_SLABEL;
            CTX.ip += CTX.slab;
            break; //goto pending_exception_check;
        }

        VM_ENTRY(MP_BC_POP_JUMP_IF_TRUE): {
            VM_DECODE_SLABEL;
            if (mp_obj_is_true(VM_POP())) {
                CTX.ip += CTX.slab;
            }
            break; //goto pending_exception_check;
        }

        VM_ENTRY(MP_BC_POP_JUMP_IF_FALSE): {
            VM_DECODE_SLABEL;
            if (!mp_obj_is_true(VM_POP())) {
                CTX.ip += CTX.slab;
            }
            break; //goto pending_exception_check;
        }

        VM_ENTRY(MP_BC_JUMP_IF_TRUE_OR_POP): {
            VM_DECODE_SLABEL;
            if (mp_obj_is_true(VM_TOP())) {
                CTX.ip += CTX.slab;
            } else {
                CTX.sp--;
            }
            break; //goto pending_exception_check;
        }

        VM_ENTRY(MP_BC_JUMP_IF_FALSE_OR_POP): {
            VM_DECODE_SLABEL;
            if (mp_obj_is_true(VM_TOP())) {
                CTX.sp--;
            } else {
                CTX.ip += CTX.slab;
            }
            break; //goto pending_exception_check;
        }

        VM_ENTRY(MP_BC_POP_EXCEPT_JUMP): {
            assert(CTX.exc_sp >= CTX.exc_stack);
            VM_POP_EXC_BLOCK();
            VM_DECODE_ULABEL;
            CTX.ip += CTX.ulab;
            break; //goto pending_exception_check;
        }

        VM_ENTRY(MP_BC_BUILD_TUPLE): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_UINT;
            CTX.sp -= unum - 1;
            VM_SET_TOP(mp_obj_new_tuple(unum, CTX.sp));
            continue;
        }

        VM_ENTRY(MP_BC_BUILD_LIST): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_UINT;
            CTX.sp -= unum - 1;
            VM_SET_TOP(mp_obj_new_list(unum, CTX.sp));
            continue;
        }

        VM_ENTRY(MP_BC_BUILD_MAP): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_UINT;
            VM_PUSH(mp_obj_new_dict(unum));
            continue;
        }

        VM_ENTRY(MP_BC_STORE_MAP): {
            MARK_EXC_IP_SELECTIVE();
            CTX.sp -= 2;
            mp_obj_dict_store(CTX.sp[0], CTX.sp[2], CTX.sp[1]);
            continue;
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
            continue;
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
            continue;
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
                    continue; // goto VM_DISPATCH_loop; // run the exception handler
                }
                VM_POP_EXC_BLOCK();
            }
            CTX.ip = (const byte*)MP_OBJ_TO_PTR(VM_POP()); // pop destination ip for jump
            if (unum != 0) {
                // pop the exhausted iterator
                CTX.sp -= MP_OBJ_ITER_BUF_NSLOTS;
            }
            break; //goto pending_exception_check;
        }

        VM_ENTRY(MP_BC_SETUP_EXCEPT):
        VM_ENTRY(MP_BC_SETUP_FINALLY): {
            MARK_EXC_IP_SELECTIVE();
            #if SELECTIVE_EXC_IP
            VM_PUSH_EXC_BLOCK((code_state->ip[-1] == MP_BC_SETUP_FINALLY) ? 1 : 0);
            #else
            VM_PUSH_EXC_BLOCK((CTX.code_state->ip[0] == MP_BC_SETUP_FINALLY) ? 1 : 0);
            #endif
            continue;
        }

        VM_ENTRY(MP_BC_END_FINALLY): {
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
            continue;
        }


        VM_ENTRY(MP_BC_GET_ITER): {
            MARK_EXC_IP_SELECTIVE();
            VM_SET_TOP(mp_getiter(VM_TOP(), NULL));
            continue;
        }

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
            continue;
        }

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
            continue;
        }

        VM_ENTRY(MP_BC_UNPACK_SEQUENCE): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_UINT;
            mp_unpack_sequence(CTX.sp[0], unum, CTX.sp);
            CTX.sp += unum - 1;
            continue;
        }

        VM_ENTRY(MP_BC_UNPACK_EX): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_UINT;
            mp_unpack_ex(CTX.sp[0], unum, CTX.sp);
            CTX.sp += (unum & 0xff) + ((unum >> 8) & 0xff);
            continue;
        }

        VM_ENTRY(MP_BC_MAKE_FUNCTION): {
            VM_DECODE_PTR;
            VM_PUSH(mp_make_function_from_raw_code(CTX.ptr, MP_OBJ_NULL, MP_OBJ_NULL));
            continue;
        }

        VM_ENTRY(MP_BC_MAKE_FUNCTION_DEFARGS): {
            VM_DECODE_PTR;
            // Stack layout: def_tuple def_dict <- TOS
            mp_obj_t def_dict = VM_POP();
            VM_SET_TOP(mp_make_function_from_raw_code(CTX.ptr, VM_TOP(), def_dict));
            continue;
        }

        VM_ENTRY(MP_BC_MAKE_CLOSURE): {
            VM_DECODE_PTR;
            size_t n_closed_over = *CTX.ip++;
            // Stack layout: closed_overs <- TOS
            CTX.sp -= n_closed_over - 1;
            VM_SET_TOP(mp_make_closure_from_raw_code(CTX.ptr, n_closed_over, CTX.sp));
            continue;
        }

        VM_ENTRY(MP_BC_MAKE_CLOSURE_DEFARGS): {
            VM_DECODE_PTR;
            size_t n_closed_over = *CTX.ip++;
            // Stack layout: def_tuple def_dict closed_overs <- TOS
            CTX.sp -= 2 + n_closed_over - 1;
            VM_SET_TOP(mp_make_closure_from_raw_code(CTX.ptr, 0x100 | n_closed_over, CTX.sp));
            continue;
        }

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
            continue;
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
                    continue; //goto VM_DISPATCH_loop;
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
            CTX.switch_break_for =1 ; break; //return MP_VM_RETURN_NORMAL;
            // no continuation

        VM_ENTRY(MP_BC_YIELD_VALUE):
//yield:
            nlr_pop();
            CTX.code_state->ip = CTX.ip;
            CTX.code_state->sp = CTX.sp;
            CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);
            CTX.vm_return_kind =  MP_VM_RETURN_YIELD;
            CTX.switch_break_for =1 ; break; //return MP_VM_RETURN_YIELD;
            // no continuation


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
                clog("mpsl:842 mp_resume + exc");
                ret_kind = mp_resume(VM_TOP(), MP_OBJ_NULL, t_exc, &ret_value);
            } else {
                clog("mpsl:845 mp_resume");
                ret_kind = mp_resume(VM_TOP(), send_value, MP_OBJ_NULL, &ret_value);
            }

            if (ret_kind == MP_VM_RETURN_YIELD) {
                fprintf(stderr,"mpsl:1061 yield from\n");
                CTX.ip--;
                VM_PUSH(ret_value);
                //goto yield;
//inline
        nlr_pop();
        CTX.code_state->ip = CTX.ip;
        CTX.code_state->sp = CTX.sp;
        CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);
        CTX.vm_return_kind =  MP_VM_RETURN_YIELD;
        CTX.switch_break_for =1 ; break; //return MP_VM_RETURN_YIELD;

            }  // no continuation

            if (ret_kind == MP_VM_RETURN_NORMAL) {
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
                continue;
            } // no continuation


            assert(ret_kind == MP_VM_RETURN_EXCEPTION);
            // Pop exhausted gen
            CTX.sp--;
            if (EXC_MATCH(ret_value, MP_OBJ_FROM_PTR(&mp_type_StopIteration))) {
                VM_PUSH(mp_obj_exception_get_value(ret_value));
                // If we injected GeneratorExit downstream, then even
                // if it was swallowed, we re-raise GeneratorExit
                GENERATOR_EXIT_IF_NEEDED(t_exc);
                continue;
            }  // no continuation

            RAISE(ret_value);
        }


#if MICROPY_PY_BUILTINS_SET
        VM_ENTRY(MP_BC_BUILD_SET): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_UINT;
            CTX.sp -= unum - 1;
            VM_SET_TOP(mp_obj_new_set(unum, CTX.sp));
            continue;
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
            continue;
        }
#endif

        #include "vm/bc_for_iter.c"

        #include "vm/bc_call_function_var_kw.c"

#include "vm/bc_call_method.c"  // FAIL

#include "vm/bc_call_function.c"  // FAIL

#include "vm/bc_import.c" // FAIL


        VM_ENTRY(MP_BC_RAISE_VARARGS): {
clog("mpsl:998 MP_BC_RAISE_VARARGS\n");
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

        default: {
            if (CTX.ip[-1] < MP_BC_LOAD_CONST_SMALL_INT_MULTI + 64) {
                VM_PUSH(MP_OBJ_NEW_SMALL_INT((mp_int_t)CTX.ip[-1] - MP_BC_LOAD_CONST_SMALL_INT_MULTI - 16));
                continue;
            }  // no continuation

            if (CTX.ip[-1] < MP_BC_LOAD_FAST_MULTI + 16) {
                obj_shared = CTX.fastn[MP_BC_LOAD_FAST_MULTI - (mp_int_t)CTX.ip[-1]];
                LOAD_CHECK();
                VM_PUSH(obj_shared);
                continue;
            }  // no continuation

            if (CTX.ip[-1] < MP_BC_STORE_FAST_MULTI + 16) {
                CTX.fastn[MP_BC_STORE_FAST_MULTI - (mp_int_t)CTX.ip[-1]] = VM_POP();
                continue;
            }  // no continuation

            if (CTX.ip[-1] < MP_BC_UNARY_OP_MULTI + MP_UNARY_OP_NUM_BYTECODE) {
                VM_SET_TOP(mp_unary_op(CTX.ip[-1] - MP_BC_UNARY_OP_MULTI, VM_TOP()));
                continue;
            }  // no continuation

            if (CTX.ip[-1] < MP_BC_BINARY_OP_MULTI + MP_BINARY_OP_NUM_BYTECODE) {
                mp_obj_t rhs = VM_POP();
                mp_obj_t lhs = VM_TOP();
                VM_SET_TOP(mp_binary_op(CTX.ip[-1] - MP_BC_BINARY_OP_MULTI, lhs, rhs));
                continue;
            }  // no continuation

            mp_obj_t obj = mp_obj_new_exception_msg(&mp_type_NotImplementedError, "byte code not implemented");
            nlr_pop();
            CTX.code_state->state[0] = obj;
            CTX.vm_return_kind =  MP_VM_RETURN_EXCEPTION;
            CTX.switch_break_for =1 ; //goto VM_mp_execute_bytecode_return;
        }

    } // switch



