goto stackmess_skip;

#define FUN_NAME qstr_str(mp_obj_fun_get_name(CTX.self_in))

SUB_call_function_n_kw: {
    mp_obj_type_t *type = mp_obj_get_type(CTX.self_in);

    //clog("  >>>>>>>>> call_function_n_kw(%d)->%s  %p %p", local_ptr,fun_name, *type->call, &fun_bc_call );
    //clog("    >>> call_function_n_kw(%s)  @%d",fun_name, local_ptr)
    if (type->call != NULL) {
        if (*type->call == &fun_bc_call)
            if (strlen(FUN_NAME))
                if (strcmp(FUN_NAME, "upper")) { // && strcmp(FUN_NAME,"importer")) {
                    clog("       !!!!!!!!!!!!!!!");
                    BRANCH(VM_fun_bc_call, CTX_call_function_n_kw_resume, FUN_NAME);
                }
        RETVAL = type->call(CTX.self_in, CTX.n_args, CTX.n_kw, CTX.args);

    } else {

        if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
            mp_raise_TypeError("object not callable");
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                "'%s' object isn't callable", type));
        }
    }
CTX_call_function_n_kw_resume:
    clog("    <## call_function_n_kw(%s)  #%d", FUN_NAME, CTX.pointer);
    RETURN;
}


#undef FUN_NAME






stackmess_skip:;
