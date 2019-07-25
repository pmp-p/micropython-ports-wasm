// mp_vm_return_kind_t mpsl_obj_gen_resume(mp_obj_t obj, mp_obj_t send_value, mp_obj_t throw_value, mp_obj_t *ret_val)

    MP_STACK_CHECK();

    mp_check_self(mp_obj_is_type(CTX.self_in, &mp_type_gen_instance));

    CTX.generator = MP_OBJ_TO_PTR(CTX.generator);

    if (CTX.generator->code_state.ip == 0) {
        // Trying to resume already stopped generator
        clog("THIS IS PROBLEMATIC");
        CTX.return_value = MP_OBJ_STOP_ITERATION;
        CTX.vm_return_kind = MP_VM_RETURN_NORMAL;
        goto return_VM_mpsl_obj_gen_resume;
    }


    if (CTX.generator->code_state.sp == CTX.generator->code_state.state - 1) {
        if (CTX.send_value != mp_const_none) {
            mp_raise_TypeError("can't send non-None value to a just-started generator");
        }
    } else {
        #if MICROPY_PY_GENERATOR_PEND_THROW
        // If exception is pending (set using .pend_throw()), process it now.
        if (*CTX.generator->code_state.sp != mp_const_none) {
            throw_value = *CTX.generator->code_state.sp;
            *CTX.generator->code_state.sp = MP_OBJ_NULL;
        } else
        #endif
        {
            *CTX.generator->code_state.sp = CTX.send_value;
        }
    }

    // We set self->globals=NULL while executing, for a sentinel to ensure the generator
    // cannot be reentered during execution
    if (CTX.generator->globals == NULL) {
        mp_raise_ValueError("generator already executing");
    }

    // Set up the correct globals context for the generator and execute it
    CTX.generator->code_state.old_globals = mp_globals_get();
    mp_globals_set(CTX.generator->globals);
    CTX.generator->globals = NULL;

    #if MICROPY_EMIT_NATIVE
    if (CTX.generator->code_state.exc_sp == NULL) {
        // A native generator, with entry point 2 words into the "bytecode" pointer
        typedef uintptr_t (*mp_fun_native_gen_t)(void*, mp_obj_t);
        mp_fun_native_gen_t fun = MICROPY_MAKE_POINTER_CALLABLE(
            (const void*)(CTX.generator->code_state.fun_bc->bytecode + 2 * sizeof(uintptr_t))
        );
        CTX.vm_return_kind = fun((void*)&self->code_state, throw_value);
    } else {
    #endif

    #if 1

        // A bytecode generator
        // =============================================================================================================
        CTX.vm_return_kind = mp_execute_bytecode(&CTX.generator->code_state, CTX.throw_value);
        CTX.generator->globals = mp_globals_get();
        mp_globals_set(CTX.generator->code_state.old_globals);
        goto VM_mpsl_obj_gen_resume_skip;
    #else
// TODO:CTX
        //    &self->code_state -> CTX.code_state
        //        throw_value -> inject_exc

        ctx_get(JMP_VM_mp_execute_bytecode, JMP_VM_mpsl_obj_gen_resume );
        NEXT.obj_shared  = obj_shared;
        NEXT.self_in = obj;
        NEXT.code_state = &self->code_state ;
        // NEXT.inject_exc = throw_value ; // defaults to MP_OBJ_NULL
        ctx_push();
        goto VM_syscall;
    #endif


// =============================================================================================================
#if MICROPY_EMIT_NATIVE
    }
#endif

VM_mpsl_obj_gen_resume:
    //ctx_pop();
    mp_globals_set(CTX.generator->code_state.old_globals);

VM_mpsl_obj_gen_resume_skip:


    switch (CTX.vm_return_kind) {
        case MP_VM_RETURN_NORMAL:
        default:
            // Explicitly mark generator as completed. If we don't do this,
            // subsequent next() may re-execute statements after last yield
            // again and again, leading to side effects.
            CTX.generator->code_state.ip = 0;
            CTX.return_value = *CTX.generator->code_state.sp;
            break;

        case MP_VM_RETURN_YIELD:
            CTX.return_value = *CTX.generator->code_state.sp;
            #if MICROPY_PY_GENERATOR_PEND_THROW
            *CTX.generator->code_state.sp = mp_const_none;
            #endif
            break;

        case MP_VM_RETURN_EXCEPTION: {
            CTX.generator->code_state.ip = 0;
            CTX.return_value = CTX.generator->code_state.state[0];
            // PEP479: if StopIteration is raised inside a generator it is replaced with RuntimeError
            if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(mp_obj_get_type(CTX.return_value)), MP_OBJ_FROM_PTR(&mp_type_StopIteration))) {
                CTX.return_value = mp_obj_new_exception_msg(&mp_type_RuntimeError, "generator raised StopIteration");
            }
            break;
        }
    }

return_VM_mpsl_obj_gen_resume:;

if (show_os_loop(-1))
    fprintf(stderr,"return mpsl_obj_gen_resume\n");

#undef self
