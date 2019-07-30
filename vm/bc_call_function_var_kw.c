// OK

VM_ENTRY(MP_BC_CALL_FUNCTION_VAR_KW): {
    MARK_EXC_IP_SELECTIVE();
    VM_DECODE_UINT;
    // unum & 0xff == n_positional
    // (unum >> 8) & 0xff == n_keyword
    // We have following stack layout here:
    // fun arg0 arg1 ... kw0 val0 kw1 val1 ... seq dict <- TOS
    CTX.sp -= (unum & 0xff) + ((unum >> 7) & 0x1fe) + 2;
    if (mp_obj_get_type(*CTX.sp) == &mp_type_fun_bc) {
        CTX.code_state->ip = CTX.ip;
        CTX.code_state->sp = CTX.sp;
        CTX.code_state->exc_sp = MP_TAGPTR_MAKE(CTX.exc_sp, 0);

        mp_call_args_t out_args;
        mp_call_prepare_args_n_kw_var(false, unum, CTX.sp, &out_args);

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

    ctx_get_next();

    static mp_call_args_t out_args;
    mp_call_prepare_args_n_kw_var(false, unum, CTX.sp, &out_args);

    NEXT.args = out_args.args;
    NEXT.alloc = out_args.n_alloc * sizeof(mp_obj_t) ;

    NEXT.self_in = out_args.fun;
    NEXT.n_args = out_args.n_args;
    NEXT.n_kw = out_args.n_kw;

#if 1
clog(">>> bc_call_method_n_kw_var");
    GOSUB(SUB_call_function_n_kw, RET_call_function_var_kw, "BC_CALL_FUNCTION_VAR_KW");
RET_call_function_var_kw:
    if (CTX.sub_alloc)
        mp_nonlocal_free(CTX.sub_args, CTX.sub_alloc);
#else
    SUBVAL = mpsl_call_function_n_kw(out_args.fun, out_args.n_args, out_args.n_kw, out_args.args);
    mp_nonlocal_free(*NEXT.args, NEXT.alloc);
    ctx_release();

    //SUBVAL = mpsl_call_method_n_kw_var(false, unum, CTX.sp);
#endif
clog("<<< bc_call_method_n_kw_var_return");

    VM_SET_TOP(SUBVAL);
    VM_DISPATCH();
}
