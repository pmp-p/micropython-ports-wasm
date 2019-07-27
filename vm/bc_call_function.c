// OK+

#define FUN_NAME qstr_str(mp_obj_fun_get_name(NEXT.self_in))

VM_ENTRY(MP_BC_CALL_FUNCTION): {
    MARK_EXC_IP_SELECTIVE();
    VM_DECODE_UINT;
    CTX.sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe);

    if (mp_obj_get_type(*CTX.sp) == &mp_type_fun_bc) {
        CTX.code_state->ip = CTX.ip;
        CTX.code_state->sp = CTX.sp;
        CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);
        mp_code_state_t *new_state = mp_obj_fun_bc_prepare_codestate(*CTX.sp, unum & 0xff, (unum >> 8) & 0xff, CTX.sp + 1);
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

    ctx_get_next();
    NEXT.self_in = *CTX.sp;
    NEXT.n_args = unum & 0xff;
    NEXT.n_kw =  (unum >> 8) & 0xff;
    NEXT.args = CTX.sp + 1;

    if ( strlen(FUN_NAME)>1 )
        GOSUB(SUB_call_function_n_kw, RET_call_function, FUN_NAME );
    else
        GOSUB(SUB_call_function_n_kw, RET_call_function, "?BC_CALL_FUNCTION?" );

RET_call_function:
    VM_SET_TOP(SUBVAL);
    VM_DISPATCH();

}


#undef FUN_NAME
