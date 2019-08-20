exception_handler:
    // exception occurred
    clog("mpsl:1204 exception_handler");
    #if MICROPY_PY_SYS_EXC_INFO
    MP_STATE_VM(cur_exception) = nlr.ret_val;
    #endif

    #if SELECTIVE_EXC_IP
    // with selective ip, we store the ip 1 byte past the opcode, so move ptr back
    CTX.code_state->ip -= 1;
    #endif

    if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(((mp_obj_base_t*)nlr.ret_val)->type), MP_OBJ_FROM_PTR(&mp_type_StopIteration))) {
        clog("mpsl:1226 exception_handler-test-done");
        if (CTX.code_state->ip) {
            // check if it's a StopIteration within a for block
            if (*CTX.code_state->ip == MP_BC_FOR_ITER) {
                const byte *exip = CTX.code_state->ip + 1;
                EX_DECODE_ULABEL; // the jump offset if iteration finishes; for labels are always forward
                CTX.code_state->ip = exip + ulab; // jump to after for-block
                CTX.code_state->sp -= MP_OBJ_ITER_BUF_NSLOTS; // pop the exhausted iterator
                GOTO_OUTER_VM_DISPATCH = 1;continue;
            } // no continuation

            if (*CTX.code_state->ip == MP_BC_YIELD_FROM) {
                // StopIteration inside yield from call means return a value of
                // yield from, so inject exception's value as yield from's result
                // (Instead of stack pop then push we just replace exhausted gen with value)
                *CTX.code_state->sp = mp_obj_exception_get_value(MP_OBJ_FROM_PTR(nlr.ret_val));
                CTX.code_state->ip++; // yield from is over, move to next instruction
                GOTO_OUTER_VM_DISPATCH = 1;continue;

            } // no continuation
            clog("ITER/YIELD/[??????]");
        } else clog("CTX.code_state->ip ??????");
        clog("mpsl:1238 continue");
    }

//FATAL("mpsl:1226 skipping bad exception_handler");

unwind_loop:
    // set file and line number that the exception occurred at
    // TODO: don't set traceback for exceptions re-raised by END_FINALLY.
    // But consider how to handle nested exceptions.
    if (nlr.ret_val != &mp_const_GeneratorExit_obj) {
        const byte *exip = CTX.code_state->fun_bc->bytecode;
        exip = mp_decode_uint_skip(exip); // skip n_state
        exip = mp_decode_uint_skip(exip); // skip n_exc_stack
        exip++; // skip scope_params
        exip++; // skip n_pos_args
        exip++; // skip n_kwonly_args
        exip++; // skip n_def_pos_args
        size_t bc = CTX.code_state->ip - exip;
        size_t code_info_size = mp_decode_uint_value(exip);
        exip = mp_decode_uint_skip(exip); // skip code_info_size
        bc -= code_info_size;
        #if MICROPY_PERSISTENT_CODE
        qstr block_name = exip[0] | (exip[1] << 8);
        qstr source_file = exip[2] | (exip[3] << 8);
        exip += 4;
        #else
        qstr block_name = mp_decode_uint_value(exip);
        exip = mp_decode_uint_skip(exip);
        qstr source_file = mp_decode_uint_value(exip);
        exip = mp_decode_uint_skip(exip);
        #endif
        size_t source_line = 1;
        size_t c;
        while ((c = *exip)) {
            size_t b, l;
            if ((c & 0x80) == 0) {
                // 0b0LLBBBBB encoding
                b = c & 0x1f;
                l = c >> 5;
                exip += 1;
            } else {
                // 0b1LLLBBBB 0bLLLLLLLL encoding (l's LSB in second byte)
                b = c & 0xf;
                l = ((c << 4) & 0x700) | exip[1];
                exip += 2;
            }
            if (bc >= b) {
                bc -= b;
                source_line += l;
            } else {
                // found source line corresponding to bytecode offset
                break;
            }
        }
        mp_obj_exception_add_traceback(MP_OBJ_FROM_PTR(nlr.ret_val), source_file, source_line, block_name);
    } else clog("vm:1424 mp_const_GeneratorExit_obj");

    while ((CTX.exc_sp >= CTX.exc_stack) && (CTX.exc_sp->handler <= CTX.code_state->ip)) {

        // nested exception

        assert(CTX.exc_sp >= CTX.exc_stack);

        // TODO make a proper message for nested exception
        // at the moment we are just raising the very last exception (the one that caused the nested exception)

        // move up to previous exception handler
        VM_POP_EXC_BLOCK();
    }

    if (CTX.exc_sp >= CTX.exc_stack) {
        // catch exception and pass to byte code
        CTX.code_state->ip = CTX.exc_sp->handler;
        mp_obj_t *exsp = MP_TAGPTR_PTR(CTX.exc_sp->val_sp);
        // save this exception in the stack so it can be used in a reraise, if needed
        CTX.exc_sp->prev_exc = nlr.ret_val;
        // push exception object so it can be handled by bytecode
        VM_PUSH(MP_OBJ_FROM_PTR(nlr.ret_val));
        CTX.code_state->sp = exsp;

    #if MICROPY_STACKLESS
    } else if (CTX.code_state->prev != NULL) {
        mp_globals_set(CTX.code_state->old_globals);
        mp_code_state_t *new_code_state = CTX.code_state->prev;
        #if MICROPY_ENABLE_PYSTACK
        // Free code_state, and args allocated by mp_call_prepare_args_n_kw_var
        // (The latter is implicitly freed when using pystack due to its LIFO nature.)
        // The sizeof in the following statement does not include the size of the variable
        // part of the struct.  This arg is anyway not used if pystack is enabled.
        mp_nonlocal_free(CTX.code_state, sizeof(mp_code_state_t));
        #endif
        CTX.code_state = new_code_state;
        size_t n_state = mp_decode_uint_value(CTX.code_state->fun_bc->bytecode);
        CTX.fastn = &CTX.code_state->state[n_state - 1];
        CTX.exc_stack = (mp_exc_stack_t*)(CTX.code_state->state + n_state);
        // variables that are visible to the exception handler (declared volatile)
        CTX.exc_sp = MP_TAGPTR_PTR(CTX.code_state->exc_sp); // stack grows up, exc_sp points to top of stack
        goto unwind_loop;

    #endif
    } else {
        // propagate exception to higher level
        // Note: ip and sp don't have usable values at this point
        CTX.code_state->state[0] = MP_OBJ_FROM_PTR(nlr.ret_val); // put exception here because sp is invalid
        CTX.vm_return_kind = MP_VM_RETURN_EXCEPTION;
        goto VM_mp_execute_bytecode_return;
    }
