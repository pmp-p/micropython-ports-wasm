

static int VM_mp_call_function = 0;

// args contains, eg: arg0  arg1  key0  value0  key1  value1
mp_obj_t mpsl_call_function_n_kw(mp_obj_t fun_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // TODO improve this: fun object can specify its type and we parse here the arguments,
    // passing to the function arrays of fixed and keyword arguments

    //DEBUG_OP_printf("calling function %p(n_args=" UINT_FMT ", n_kw=" UINT_FMT ", args=%p)\n", fun_in, n_args, n_kw, args);

    // get the type
    mp_obj_type_t *type = mp_obj_get_type(fun_in);

    // do the call
    if (type->call != NULL) {

        if (show_os_loop(-1)) fprintf(stderr,"    mpsl_call_function_n_kw(%d)\n",VM_mp_call_function++);
        mp_obj_t result = type->call(fun_in, n_args, n_kw, args);
        if (show_os_loop(-1)) fprintf(stderr,"    mpsl_call_function_n_kw_return(%d)\n",--VM_mp_call_function);
        return result;
    }

    if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
        mp_raise_TypeError("object not callable");
    } else {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
            "'%s' object isn't callable", mp_obj_get_type_str(fun_in)));
    }
}


mp_obj_t mpsl_call_method_n_kw_var(bool have_self, size_t n_args_n_kw, const mp_obj_t *args) {
    mp_call_args_t out_args;
    mp_call_prepare_args_n_kw_var(have_self, n_args_n_kw, args, &out_args);

    mp_obj_t res = mpsl_call_function_n_kw(out_args.fun, out_args.n_args, out_args.n_kw, out_args.args);
    mp_nonlocal_free(out_args.args, out_args.n_alloc * sizeof(mp_obj_t));

    return res;
}


#if 1
// may return MP_OBJ_STOP_ITERATION as an optimisation instead of raise StopIteration()
// may also raise StopIteration()
mp_obj_t mpsl_iternext_allow_raise(mp_obj_t o_in) {
    mp_obj_type_t *type = mp_obj_get_type(o_in);
    if (type->iternext != NULL) {
        return type->iternext(o_in);
    } else {
        // check for __next__ method
        mp_obj_t dest[2];
        mp_load_method_maybe(o_in, MP_QSTR___next__, dest);
        if (dest[0] != MP_OBJ_NULL) {
            // __next__ exists, call it and return its result
if (show_os_loop(-1)) fprintf(stderr, "    766:mp_call_method_n_kw\n");
            mp_obj_t next = mp_call_method_n_kw(0, 0, dest);
if (show_os_loop(-1)) fprintf(stderr, "    766:mp_call_method_n_kw_return\n");
            return next;
        } else {
            if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
                mp_raise_TypeError("object not an iterator");
            } else {
                nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                    "'%s' object isn't an iterator", mp_obj_get_type_str(o_in)));
            }
        }
    }
}
#else

// may return MP_OBJ_STOP_ITERATION as an optimisation instead of raise StopIteration()
// may also raise StopIteration()
mp_obj_t mpsl_iternext_allow_raise(mp_obj_t o_in) {
    mp_obj_type_t *type = mp_obj_get_type(o_in);
    if (type->iternext != NULL) {
        return type->iternext(o_in);
    } else {
        // check for __next__ method
        mp_obj_t dest[2];
        mp_load_method_maybe(o_in, MP_QSTR___next__, dest);
        if (dest[0] != MP_OBJ_NULL) {
            // __next__ exists, call it and return its result
            return mp_call_method_n_kw(0, 0, dest);
        } else {
            if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
                mp_raise_TypeError("object not an iterator");
            } else {
                nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                    "'%s' object isn't an iterator", mp_obj_get_type_str(o_in)));
            }
        }
    }
}





#endif
