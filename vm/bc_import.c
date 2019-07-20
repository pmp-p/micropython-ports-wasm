        VM_ENTRY(MP_BC_IMPORT_NAME): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            mp_obj_t obj = POP();
            SET_TOP(mp_import_name(qst, obj, TOP()));

if ( !strcmp(qstr_str(qst),"syscall") ) {
    fprintf(stderr,"iter:1140 import->pause\n");
    mpi_ctx[ctx_current].vmloop_state = VM_PAUSED;
    TRACE(ip);
    VM_MARK_EXC_IP_GLOBAL();
    nlr_pop();
    goto VM_paused;
} else
    fprintf(stderr,"iter:1139 import [%s]\n", qstr_str(qst) );
            VM_DISPATCH();
        }

        VM_ENTRY(MP_BC_IMPORT_FROM): {
            MARK_EXC_IP_SELECTIVE();
            VM_DECODE_QSTR;
            mp_obj_t obj = mp_import_from(TOP(), qst);
            PUSH(obj);
            VM_DISPATCH();
        }

        VM_ENTRY(MP_BC_IMPORT_STAR): {
            MARK_EXC_IP_SELECTIVE();
            mp_import_all(POP());
            VM_DISPATCH();
        }
