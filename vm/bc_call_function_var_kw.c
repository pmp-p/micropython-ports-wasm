#if VM_SHOW_TRACE
                VM_ENTRY(MP_BC_CALL_FUNCTION_VAR_KW): {
                    MARK_EXC_IP_SELECTIVE();
                    DECODE_UINT;
                    // unum & 0xff == n_positional
                    // (unum >> 8) & 0xff == n_keyword
                    // We have following stack layout here:
                    // fun arg0 arg1 ... kw0 val0 kw1 val1 ... seq dict <- TOS
                    sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe) + 2;
                    #if MICROPY_STACKLESS
                    if (mp_obj_get_type(*sp) == &mp_type_fun_bc) {
                        CTX.code_state->ip = ip;
                        CTX.code_state->sp = sp;
                        CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);

                        mp_call_args_t out_args;
                        mp_call_prepare_args_n_kw_var(false, unum, sp, &out_args);

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
static int VM_mp_call_method_n_kw_var = 0;
VM_mp_call_method_n_kw_var++;
if (show_os_loop(-1)) fprintf(stderr,"iter:917 mp_call_method_n_kw_var(%d)\n", VM_mp_call_method_n_kw_var);
                    mp_obj_t VM_result_2 = mp_call_method_n_kw_var(false, unum, sp);

if (show_os_loop(-1)) fprintf(stderr,"iter:917 RETURN FROM mp_call_method_n_kw_var(%d)\n", VM_mp_call_method_n_kw_var);

VM_2:
VM_mp_call_method_n_kw_var--;
                    SET_TOP(VM_result_2);
                    VM_DISPATCH();
                }

#else

                VM_ENTRY(MP_BC_CALL_FUNCTION_VAR_KW): {
                    MARK_EXC_IP_SELECTIVE();
                    DECODE_UINT;
                    // unum & 0xff == n_positional
                    // (unum >> 8) & 0xff == n_keyword
                    // We have following stack layout here:
                    // fun arg0 arg1 ... kw0 val0 kw1 val1 ... seq dict <- TOS
                    sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe) + 2;
                    if (mp_obj_get_type(*sp) == &mp_type_fun_bc) {
                        CTX.code_state->ip = ip;
                        CTX.code_state->sp = sp;
                        CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);

                        mp_call_args_t out_args;
                        mp_call_prepare_args_n_kw_var(false, unum, sp, &out_args);

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

                    mp_obj_t VM_result_2 = mp_call_method_n_kw_var(false, unum, sp);
VM_2:
                    SET_TOP(VM_result_2);
                    VM_DISPATCH();
                }
#endif
