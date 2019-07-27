#if 0
VM_ENTRY(MP_BC_FOR_ITER): {
    MARK_EXC_IP_SELECTIVE();
    VM_DECODE_ULABEL; // the jump offset if iteration finishes; for labels are always forward
    CTX.code_state->sp = CTX.sp;

    static mp_obj_t return_value = MP_OBJ_NULL;;
    static mp_obj_t obj ;
    static mp_obj_t send_value = mp_const_none;
    static mp_obj_t throw_value = MP_OBJ_NULL;
    static mp_obj_type_t *type;

    if (CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 1] == MP_OBJ_NULL) {
        obj = CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 2];
    } else {
        obj = MP_OBJ_FROM_PTR(&CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 1]);
    }

    type = mp_obj_get_type(obj);

    if (type->iternext != NULL) {
        if ((mp_fun_1_t)type->iternext == gen_instance_iternext_ptr() ) {

if (show_os_loop(-1)) fprintf(stderr, "    BC_FOR_ITER:gen_instance_iternext\n"  );

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

if (show_os_loop(-1)) fprintf(stderr, "    BC_FOR_ITER:gen_instance_iternext_return\n"  );

        } else {
            if (show_os_loop(-1)) fprintf(stderr, "    BC_FOR_ITER:type->iternext\n"  );
            return_value = type->iternext(obj);
            if (show_os_loop(-1)) fprintf(stderr, "    BC_FOR_ITER:type->iternext_return\n");
        }

    } else {
        // check for __next__ method
        mp_obj_t dest[2];
        mp_load_method_maybe(obj, MP_QSTR___next__, dest);
        if (dest[0] != MP_OBJ_NULL) {
            // __next__ exists, call it and return its result
if (show_os_loop(-1)) fprintf(stderr, "    BC_FOR_ITER:mp_call_method_n_kw\n");
        return_value = mp_call_method_n_kw(0, 0, dest);
if (show_os_loop(-1)) fprintf(stderr, "    BC_FOR_ITER:mp_call_method_n_kw_return\n");
        } else {
            if (MICROPY_ERROR_REPORTING == MICROPY_ERROR_REPORTING_TERSE) {
                mp_raise_TypeError("object not an iterator");
            } else {
                nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_TypeError,
                    "'%s' object isn't an iterator", mp_obj_get_type_str(obj)));
            }
        }
    }

    if (return_value == MP_OBJ_STOP_ITERATION) {
        CTX.sp -= MP_OBJ_ITER_BUF_NSLOTS; // pop the exhausted iterator
        CTX.ip += CTX.ulab; // jump to after for-block
    } else {
        VM_PUSH(return_value); // push the next iteration value
    }
    VM_DISPATCH();
}

#else

    VM_ENTRY(MP_BC_FOR_ITER): {
        MARK_EXC_IP_SELECTIVE();
        VM_DECODE_ULABEL; // the jump offset if iteration finishes; for labels are always forward
        CTX.code_state->sp = CTX.sp;
        mp_obj_t obj;
        if (CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 1] == MP_OBJ_NULL) {
            obj = CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 2];
        } else {
            obj = MP_OBJ_FROM_PTR(&CTX.sp[-MP_OBJ_ITER_BUF_NSLOTS + 1]);
        }
clog("    bc_for_iter:98 >> mpsl_iternext_allow_raise\n");
        mp_obj_t value = mpsl_iternext_allow_raise(obj);

clog("    bc_for_iter:101 << mpsl_iternext_allow_raise_return\n");
        if (value == MP_OBJ_STOP_ITERATION) {
            CTX.sp -= MP_OBJ_ITER_BUF_NSLOTS; // pop the exhausted iterator
            CTX.ip += CTX.ulab; // jump to after for-block
        } else {
            VM_PUSH(value); // push the next iteration value
        }
        VM_DISPATCH();
    }
#endif
