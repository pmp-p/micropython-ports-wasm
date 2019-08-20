VM_ENTRY(MP_BC_FOR_ITER): {
    MARK_EXC_IP_SELECTIVE();
    VM_DECODE_ULABEL; // the jump offset if iteration finishes; for labels are always forward
    CTX.code_state->sp = CTX.sp;

    ctx_get_next(CTX_NEW);
    NEXT.return_value = MP_OBJ_NULL;;
    NEXT.send_value = mp_const_none;
    NEXT.throw_value = MP_OBJ_NULL;

    if (CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 1] == MP_OBJ_NULL) {
        NEXT.self_in = CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 2];
    } else {
        NEXT.self_in = MP_OBJ_FROM_PTR(&CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 1]);
    }
// =============  mp_obj_t value = mpsl_iternext_allow_raise(obj); ===============

    mp_obj_type_t *type = mp_obj_get_type(NEXT.self_in);

    if (type->iternext != NULL) {


        clog(">>>  BC_FOR_ITER:gen_instance_iternext\n"  );
            RETVAL = type->iternext(NEXT.self_in);

        if ( (void*)*type->iternext == &gen_instance_iternext ) {
            clog(">>> BC_FOR_ITER:type->iternext\n"  );
#if 0
            static mp_vm_return_kind_t mpsl_obj_gen_resume;

            #include "vm/mpsl_obj_gen_resume.c"

            switch (mpsl_obj_gen_resume) {
                case MP_VM_RETURN_NORMAL:
                default:
                    // Optimize return w/o value in case generator is used in for loop
                    if (return_value == mp_const_none || return_value == MP_OBJ_STOP_ITERATION) {
                        return_value = MP_OBJ_STOP_ITERATION;
                        break;
                    } else {
                        nlr_raise(mp_obj_new_exception_args(&mp_type_StopIteration, 1, &return_value));
                        break;
                    }

                case MP_VM_RETURN_YIELD:
                    //return_value = return_value;
                    break;

                case MP_VM_RETURN_EXCEPTION:
                    nlr_raise(return_value);
            }


#endif
            clog("<<< BC_FOR_ITER:type->iternext_return\n");
        }

        clog("<<< BC_FOR_ITER:gen_instance_iternext_return\n"  );

    } else {
        // check for __next__ method
        mp_obj_t dest[2];
        mp_load_method_maybe(NEXT.self_in, MP_QSTR___next__, dest);
        if (dest[0] != MP_OBJ_NULL) {
            // __next__ exists, call it and return its result
clog(">>> BC_FOR_ITER:mp_call_method_n_kw\n");
            RETVAL = mp_call_method_n_kw(0, 0, dest);
clog("<<< BC_FOR_ITER:mp_call_method_n_kw_return\n");
        } else {
            if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
                mp_raise_TypeError("object not an iterator");
            } else {
                nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                    "'%s' object isn't an iterator", mp_obj_get_type_str(NEXT.self_in)));
            }
        }
    }

//========================== end mpsl_iternext_allow_raise(mp_obj_t o_in)  ==================
    if (RETVAL == MP_OBJ_STOP_ITERATION) {
        CTX.sp -= MP_OBJ_ITER_BUF_NSLOTS; // pop the exhausted iterator
        CTX.ip += CTX.ulab; // jump to after for-block
    } else {
        VM_PUSH(RETVAL); // push the next iteration value
    }
    ctx_free();
    continue;
}

