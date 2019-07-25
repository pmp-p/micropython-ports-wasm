VM_ENTRY(MP_BC_CALL_METHOD): {
    MARK_EXC_IP_SELECTIVE();
    VM_DECODE_UINT;
    // unum & 0xff == n_positional
    // (unum >> 8) & 0xff == n_keyword
    CTX.sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe) + 1;

    if (mp_obj_get_type(*CTX.sp) == &mp_type_fun_bc) {
        CTX.code_state->ip = CTX.ip;
        CTX.code_state->sp = CTX.sp;
        CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);

        size_t n_args = unum & 0xff;
        size_t n_kw = (unum >> 8) & 0xff;
        int adjust = (CTX.sp[1] == MP_OBJ_NULL) ? 0 : 1;

        mp_code_state_t *new_state = mp_obj_fun_bc_prepare_codestate(*CTX.sp, n_args + adjust, n_kw, CTX.sp + 2 - adjust);
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

    VM_SET_TOP(mp_call_method_n_kw(unum & 0xff, (unum >> 8) & 0xff, CTX.sp));

//    if (show_os_loop(-1))  fprintf(stderr,"    BC_CALL_METHOD\n" );
    /*
    if ( mp_obj_get_type(fun_in)->call != NULL ) {
        const char *fun_name = qstr_str(mp_obj_fun_get_name(fun_in));

        if (show_os_loop(-1))  fprintf(stderr,"     ---------- %s ----------\n", fun_name );
        if (!strcmp(fun_name,"awaited_sleep" )) {
            CTX.jump_return = JMP_VM_call_function_resume ;
            VM_MARK_EXC_IP_GLOBAL();
            goto VM_syscall;
        }
    }
*/
    VM_DISPATCH();
}


