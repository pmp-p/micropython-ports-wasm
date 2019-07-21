#if 0 //VM_SHOW_TRACE
    VM_ENTRY(MP_BC_CALL_FUNCTION): {
        MARK_EXC_IP_SELECTIVE();
        DECODE_UINT;
        // unum & 0xff == n_positional
        // (unum >> 8) & 0xff == n_keyword
        sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe);
        #if MICROPY_STACKLESS
        if (mp_obj_get_type(*sp) == &mp_type_fun_bc) {
            CTX.code_state->ip = ip;
            CTX.code_state->sp = sp;
            CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);
            mp_code_state_t *new_state = mp_obj_fun_bc_prepare_codestate(*sp, unum & 0xff, (unum >> 8) & 0xff, sp + 1);
            #if !MICROPY_ENABLE_PYSTACK
            if (new_state == NULL) {
                // Couldn't allocate codestate on heap: in the strict case raise
                // an exception, otherwise just fall through to stack allocation.
                #if MICROPY_STACKLESS_STRICT
            deep_recursion_error:
                mp_raise_recursion_depth();
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
static int VM_mp_call_function_n_kw = 0;
VM_mp_call_function_n_kw++;
if (show_os_loop(-1)) fprintf(stderr,"    BC_CALL_FUNCTION mp_call_function_n_kw(%d)\n",VM_mp_call_function_n_kw);

        {
            mp_obj_t fun_in = *sp;
            size_t n_args = unum & 0xff;
            size_t n_kw =  (unum >> 8) & 0xff;
            const mp_obj_t *args = sp + 1;

            mp_obj_t VM_result_1;

            mp_obj_type_t *type = mp_obj_get_type(fun_in);

            if (type->call != NULL) {
                #if 0
                // ctx_current = ctx_push( 1 );  //VM_1
                ctx_next = ctx_push( 1 );
                if (show_os_loop(-1)){
                   fprintf(stderr,"     ========= [%s:%s] should ctx %d push here========\n",
                        qstr_str(mp_obj_fun_get_name(fun_in)),
                        qstr_str(mp_obj_fun_get_name(type)),
                    ctx_next);
                }
                //NEXT.
                #endif

                VM_result_1 = type->call(fun_in, n_args, n_kw, args);
                #if 0
                if (show_os_loop(-1))
                    fprintf(stderr,"     ========= should have pushed ========\n");
                #endif
                goto VM_1;
            }

            if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
                mp_raise_TypeError("object not callable");
            } else {
                nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                    "'%s' object isn't callable", mp_obj_get_type_str(fun_in)));
            }

VM_1:
if (show_os_loop(-1)) fprintf(stderr,"    BC_CALL_FUNCTION mp_call_function_n_kw_return\n");

VM_mp_call_function_n_kw--;
            SET_TOP(VM_result_1);
        }
        VM_DISPATCH();
    }
#else

    VM_ENTRY(MP_BC_CALL_FUNCTION): {
        MARK_EXC_IP_SELECTIVE();
        DECODE_UINT;
        sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe);

        if (mp_obj_get_type(*sp) == &mp_type_fun_bc) {
            CTX.code_state->ip = ip;
            CTX.code_state->sp = sp;
            CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);
            mp_code_state_t *new_state = mp_obj_fun_bc_prepare_codestate(*sp, unum & 0xff, (unum >> 8) & 0xff, sp + 1);
            #if !MICROPY_ENABLE_PYSTACK
            if (new_state == NULL) {
                // Couldn't allocate codestate on heap: in the strict case raise
                // an exception, otherwise just fall through to stack allocation.
                #if MICROPY_STACKLESS_STRICT
            deep_recursion_error:
                mp_raise_recursion_depth();
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
        SET_TOP(mp_call_function_n_kw(*sp, unum & 0xff, (unum >> 8) & 0xff, sp + 1));
VM_1:
        VM_DISPATCH();
    }

#endif
