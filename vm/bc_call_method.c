#if 0
VM_ENTRY(MP_BC_CALL_METHOD): {
    MARK_EXC_IP_SELECTIVE();
    DECODE_UINT;
    // unum & 0xff == n_positional
    // (unum >> 8) & 0xff == n_keyword
    sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe) + 1;
    #if MICROPY_STACKLESS
    if (mp_obj_get_type(*sp) == &mp_type_fun_bc) {
        CTX.code_state->ip = ip;
        CTX.code_state->sp = sp;
        CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);

        size_t n_args = unum & 0xff;
        size_t n_kw = (unum >> 8) & 0xff;
        int adjust = (sp[1] == MP_OBJ_NULL) ? 0 : 1;

        mp_code_state_t *new_state = mp_obj_fun_bc_prepare_codestate(*sp, n_args + adjust, n_kw, sp + 2 - adjust);
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
    SET_TOP(mp_call_method_n_kw(unum & 0xff, (unum >> 8) & 0xff, sp));
    VM_DISPATCH();
}

#else

VM_ENTRY(MP_BC_CALL_METHOD): {
    MARK_EXC_IP_SELECTIVE();
    DECODE_UINT;
    // unum & 0xff == n_positional
    // (unum >> 8) & 0xff == n_keyword
    sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe) + 1;

    if (mp_obj_get_type(*sp) == &mp_type_fun_bc) {
        CTX.code_state->ip = ip;
        CTX.code_state->sp = sp;
        CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);

        size_t n_args = unum & 0xff;
        size_t n_kw = (unum >> 8) & 0xff;
        int adjust = (sp[1] == MP_OBJ_NULL) ? 0 : 1;

        mp_code_state_t *new_state = mp_obj_fun_bc_prepare_codestate(*sp, n_args + adjust, n_kw, sp + 2 - adjust);
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

    SET_TOP(mp_call_method_n_kw(unum & 0xff, (unum >> 8) & 0xff, sp));
    VM_DISPATCH();
}

#endif
