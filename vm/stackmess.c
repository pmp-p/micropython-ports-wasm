VM_stackmess:

if (VMOP==VMOP_NONE)
    return;

if (VMOP==VMOP_CRASH) {
    clog("KP - game over");
    //emscripten_pause_main_loop();
    emscripten_cancel_main_loop();
    return;
}

if (VMOP==VMOP_PAUSE) {
    CTX.vmloop_state = VM_PAUSED + 3;
    fprintf(stderr," - paused interpreter %d -\n", ctx_current);
    return;
}

if (VMOP==VMOP_SYSCALL) {
    CTX.vmloop_state = VM_SYSCALL;
    if (show_os_loop(-1))
        fprintf(stderr," - syscall %d -\n", ctx_current);
    return;
}

if (VMOP==VMOP_CALL) {
/* unwrapping of

mp_obj_t mp_call_function_n_kw(mp_obj_t fun_in, size_t n_args, size_t n_kw, const mp_obj_t *args)

*/

#define FUN_NAME qstr_str(mp_obj_fun_get_name(CTX.self_in))

SUB_call_function_n_kw: ;

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


            if (!strcmp(FUN_NAME,"syscall")){
                ctx_sti++;
                clog("\n  ********* STI[%d]: MUST MAKE CTX>%d INTERRUPTIBLE ************ ", ctx_sti, ctx_current);
            }

            if ( ctx_sti>0)
                BRANCH(VM_fun_bc_call, VMOP_NONE, CTX_call_function_n_kw_cli, FUN_NAME);
            else
                BRANCH(VM_fun_bc_call, VMOP_NONE, CTX_call_function_n_kw_resume, FUN_NAME);
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
            // recursive is now iterative
            GOSUB(SUB_call_function_n_kw, CTX_call_function_n_kw_free, FUN_NAME);

CTX_call_function_n_kw_free:
            if (CTX.sub_alloc)
                m_del(mp_obj_t, CTX.sub_args, CTX.sub_alloc);
            RETVAL = SUBVAL ;

            goto CTX_call_function_n_kw_resume;
            #undef self
        } // no continuation

        if (*type->call == &fun_builtin_var_call) {
            {

                #define self CTX.self_fb
                self = MP_OBJ_TO_PTR(CTX.self_in);

                // check number of arguments
                mp_arg_check_num_sig(CTX.n_args, CTX.n_kw, self->sig);

                if (self->sig & 1) {
                    // function allows keywords

                    // we create a map directly from the given args array
                    mp_map_t kw_args;
                    mp_map_init_fixed_table(&kw_args, CTX.n_kw, CTX.args + CTX.n_args);
                    clog("    [%d] <fun_builtin_var_call(%s)->fun.kw(...) '%s' %d->%d>", CTX.sub_id, self->name, FUN_NAME, CTX.parent, ctx_current);
                    RETVAL = self->fun.kw(CTX.n_args, CTX.args, &kw_args);

                } else {
                    // function takes a variable number of arguments, but no keywords
                    if ( !strcmp(self->name,"mp_builtin_next") )
                        clog("       ============= ITERATOR !!!!!!!! ===========");
                    clog("    [%d] <fun_builtin_var_call(%s)->fun.var(...) '%s' %d->%d>", CTX.sub_id, self->name, FUN_NAME, CTX.parent, ctx_current);
                    RETVAL = self->fun.var(CTX.n_args, CTX.args);
                }
                if (ctx_sti)
                    goto CTX_call_function_n_kw_cli;
                else
                    goto CTX_call_function_n_kw_resume;

            }

        } else if (*type->call == &fun_builtin_1_call) {
            clog("    [%d] <fun_builtin_1_call(...) '%s' %d->%d>", CTX.sub_id, FUN_NAME, CTX.parent, ctx_current);
        } else {
            clog("    [%d:%d] ?? %s(...) %p  %d->%d", ctx_current, CTX.sub_id, FUN_NAME, type->call, CTX.parent, ctx_current);
        }

        RETVAL = type->call(CTX.self_in, CTX.n_args, CTX.n_kw, CTX.args);
        clog("<<<<<[%d:%d] ?? %s(...) %p  %d->%d", ctx_current, CTX.sub_id, FUN_NAME, type->call, CTX.parent, ctx_current);

    CTX_call_function_n_kw_cli:
        if (!strcmp(FUN_NAME,"syscall")) ctx_sti--;

    CTX_call_function_n_kw_resume:
        clog("    End[%d:%d] '%s'(...)  %d->%d\n", ctx_current, CTX.sub_id, FUN_NAME, ctx_current, CTX.parent);
        RETURN;

    #undef FUN_NAME
    return;
}
// mp_call_function_n_kw

