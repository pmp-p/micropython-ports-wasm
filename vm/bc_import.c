VM_ENTRY(MP_BC_IMPORT_NAME): {
    MARK_EXC_IP_SELECTIVE();
    VM_DECODE_QSTR;  // qst => import [name]
    ctx_get_next();
    NEXT.qst = CTX.qst;
    NEXT.n_args = 5 ;
    NEXT.n_kw = 0;
    NEXT.argv[0] = MP_OBJ_NEW_QSTR(CTX.qst) ;
    NEXT.argv[1] = mp_const_none; // TODO should be globals
    NEXT.argv[2] = mp_const_none; // TODO should be globals
    NEXT.argv[3] = VM_POP(); // fromlist
    NEXT.argv[4] = VM_TOP();

    NEXT.args  = &NEXT.argv[0] ; //implicit

#if 0
    CTX.sub_value = mp_import_name(NEXT.qst, NEXT.argv[3], NEXT.argv[4] );
    ctx_release();
#else
    #if 1 //MICROPY_CAN_OVERRIDE_BUILTINS
    {
        mp_obj_dict_t *bo_dict = MP_STATE_VM(mp_module_builtins_override_dict);
        // lookup __import__ and call that instead of going straight to builtin implementation
        if (bo_dict != NULL) {
            mp_map_elem_t *cust_imp = mp_map_lookup(&bo_dict->map, MP_OBJ_NEW_QSTR(MP_QSTR___import__), MP_MAP_LOOKUP);
            if (cust_imp != NULL) {
            #if 1 // TODO:CTX NOT OK -> globals()
                NEXT.self_in = cust_imp->value ;
                GOSUB(SUB_call_function_n_kw, RET_import_name, qstr_str(CTX.qst));
RET_import_name:
            #else
                CTX.sub_value = mpsl_call_function_n_kw(cust_imp->value, NEXT.n_args, NEXT.n_kw, NEXT.argv);
                ctx_release();
            #endif
                goto EXIT_import_name;
            }
        }
    }
    CTX.sub_value = mp_builtin___import__(5, CTX.argv);
    #else
    CTX.sub_value = mp_builtin___import__(5, CTX.argv);
    ctx_release();
    #endif
#endif
EXIT_import_name:
    VM_SET_TOP( CTX.sub_value );

if ( !strcmp(qstr_str(CTX.qst),"syscall") ) {
    fprintf(stderr,"    BC_IMPORT import->pause\n");
    mpi_ctx[ctx_current].vmloop_state = VM_PAUSED;
    TRACE(CTX.ip);
    VM_MARK_EXC_IP_GLOBAL();
    nlr_pop();
    goto VM_paused;
} else
    fprintf(stderr,"    BC_IMPORT [%s]\n", qstr_str(CTX.qst) );

    VM_DISPATCH();
}




VM_ENTRY(MP_BC_IMPORT_FROM): {
    MARK_EXC_IP_SELECTIVE();
    VM_DECODE_QSTR;
    mp_obj_t obj = mp_import_from(VM_TOP(), CTX.qst);
    VM_PUSH(obj);
    VM_DISPATCH();
}




VM_ENTRY(MP_BC_IMPORT_STAR): {
    MARK_EXC_IP_SELECTIVE();
    mp_import_all(VM_POP());
    VM_DISPATCH();
}

