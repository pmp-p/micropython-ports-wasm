goto stackmess_skip;

/* unwrapping of

mp_obj_t mp_call_function_n_kw(mp_obj_t fun_in, size_t n_args, size_t n_kw, const mp_obj_t *args)

*/

#define FUN_NAME qstr_str(mp_obj_fun_get_name(CTX.self_in))

SUB_call_function_n_kw: {

    mp_obj_type_t *type = mp_obj_get_type(CTX.self_in);

    if ( JUMP_TYPE != TYPE_SUB )
        FATAL("ERROR: mp_call_function_n_kw is a sub not a branch");

    if (type->call == NULL) {
        if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
            mp_raise_TypeError("object not callable");
        } else {
            nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                "'%s' object isn't callable", type));
        }
        goto CTX_call_function_n_kw_resume;
    }   //no continuation


    if (*type->call == &fun_bc_call) {
        clog("    [%d] <fun_bc_call(...) '%s' %d->%d>", CTX.sub_id, FUN_NAME, CTX.parent, ctx_current);
        BRANCH(VM_fun_bc_call, CTX_call_function_n_kw_resume, FUN_NAME);

    }   //no continuation


    if (*type->call == &closure_call) {
        #define self NEXT.self_clo
        clog("    [%d] <closure_call(...) '%s' %d->%d>", CTX.sub_id, FUN_NAME, CTX.parent, ctx_current);

        ctx_get_next();

        self = MP_OBJ_TO_PTR(CTX.self_in);

        // need to concatenate closed-over-vars and args
        NEXT.alloc = self->n_closed + CTX.n_args + 2 * CTX.n_kw;

        // use heap to allocate temporary args array
        if (NEXT.alloc > 5) {
            NEXT.args = m_new(mp_obj_t, NEXT.alloc);
        } else {
            NEXT.alloc = 0;
            NEXT.args = &NEXT.argv[0];
        }

        memcpy(NEXT.args, self->closed, self->n_closed * sizeof(mp_obj_t));
        memcpy(NEXT.args + self->n_closed, CTX.args, (CTX.n_args + 2 * CTX.n_kw) * sizeof(mp_obj_t));

        NEXT.self_in = self->fun;
        NEXT.n_args = self->n_closed + CTX.n_args;
        NEXT.n_kw = CTX.n_kw ;
// BUG THERE
#if 1
#pragma message ("I WILL CRASH ON next()")
        // recursive is now iterative
        GOSUB(SUB_call_function_n_kw, CTX_call_function_n_kw_free, FUN_NAME);
CTX_call_function_n_kw_free:
        if (CTX.sub_alloc)
            m_del(mp_obj_t, CTX.sub_args, CTX.sub_alloc);
        RETVAL = SUBVAL ;

#else
        RETVAL = mpsl_call_function_n_kw(NEXT.self_in, NEXT.n_args, NEXT.n_kw, NEXT.args);
        if (NEXT.alloc)
            m_del(mp_obj_t, NEXT.args, NEXT.alloc);
        ctx_release();
#endif
        goto CTX_call_function_n_kw_resume;
        #undef self
    } // no continuation

    if (*type->call == &fun_builtin_var_call) {
        {
            clog("    [%d] <fun_builtin_var_call(...) '%s' %d->%d>", CTX.sub_id, FUN_NAME, CTX.parent, ctx_current);
            #define self CTX.self_fb
            self = MP_OBJ_TO_PTR(CTX.self_in);











        }

    } else if (*type->call == &fun_builtin_1_call) {
        clog("    [%d] <fun_builtin_1_call(...) '%s' %d->%d>", CTX.sub_id, FUN_NAME, CTX.parent, ctx_current);
    } else {
        clog("    [%d:%d] ?? %s(...) %p  %d->%d", ctx_current, CTX.sub_id, FUN_NAME, type->call, CTX.parent, ctx_current);
    }

    RETVAL = type->call(CTX.self_in, CTX.n_args, CTX.n_kw, CTX.args);
    clog("<<<<<[%d:%d] ?? %s(...) %p  %d->%d", ctx_current, CTX.sub_id, FUN_NAME, type->call, CTX.parent, ctx_current);

CTX_call_function_n_kw_resume:
    clog("    End[%d:%d] '%s'(...)  %d->%d\n", ctx_current, CTX.sub_id, FUN_NAME, ctx_current, CTX.parent);
    RETURN;
}


#undef FUN_NAME


// mp_call_function_n_kw


stackmess_skip:;
