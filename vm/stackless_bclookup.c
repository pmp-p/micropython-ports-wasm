    VM_ENTRY(MP_BC_LOAD_NAME): {
        MARK_EXC_IP_SELECTIVE();
        VM_DECODE_QSTR;
        mp_obj_t key = MP_OBJ_NEW_QSTR(CTX.qst);
        mp_uint_t x = *CTX.ip;
        if (x < mp_locals_get()->map.alloc && mp_locals_get()->map.table[x].key == key) {
            VM_PUSH(mp_locals_get()->map.table[x].value);
        } else {
            mp_map_elem_t *elem = mp_map_lookup(&mp_locals_get()->map, MP_OBJ_NEW_QSTR(CTX.qst), MP_MAP_LOOKUP);
            if (elem != NULL) {
                *(byte*)CTX.ip = (elem - &mp_locals_get()->map.table[0]) & 0xff;
                VM_PUSH(elem->value);
            } else {
                VM_PUSH(mp_load_name(MP_OBJ_QSTR_VALUE(key)));
            }
        }
        CTX.ip++;
        continue;
    }

    VM_ENTRY(MP_BC_LOAD_GLOBAL): {
        MARK_EXC_IP_SELECTIVE();
        VM_DECODE_QSTR;
        mp_obj_t key = MP_OBJ_NEW_QSTR(CTX.qst);
        mp_uint_t x = *CTX.ip;
        if (x < mp_globals_get()->map.alloc && mp_globals_get()->map.table[x].key == key) {
            VM_PUSH(mp_globals_get()->map.table[x].value);
        } else {
            mp_map_elem_t *elem = mp_map_lookup(&mp_globals_get()->map, MP_OBJ_NEW_QSTR(CTX.qst), MP_MAP_LOOKUP);
            if (elem != NULL) {
                *(byte*)CTX.ip = (elem - &mp_globals_get()->map.table[0]) & 0xff;
                VM_PUSH(elem->value);
            } else {
                VM_PUSH(mp_load_global(MP_OBJ_QSTR_VALUE(key)));
            }
        }
        CTX.ip++;
        continue;
    }

    VM_ENTRY(MP_BC_LOAD_ATTR): {
        MARK_EXC_IP_SELECTIVE();
        VM_DECODE_QSTR;
        mp_obj_t top = VM_TOP();
        if (mp_obj_is_instance_type(mp_obj_get_type(top))) {
            mp_obj_instance_t *self = MP_OBJ_TO_PTR(top);
            mp_uint_t x = *CTX.ip;
            mp_obj_t key = MP_OBJ_NEW_QSTR(CTX.qst);
            mp_map_elem_t *elem;
            if (x < self->members.alloc && self->members.table[x].key == key) {
                elem = &self->members.table[x];
            } else {
                elem = mp_map_lookup(&self->members, key, MP_MAP_LOOKUP);
                if (elem != NULL) {
                    *(byte*)CTX.ip = elem - &self->members.table[0];
                } else {
                    goto load_attr_cache_fail;
                }
            }
            VM_SET_TOP(elem->value);
            CTX.ip++;
            continue;
        }
load_attr_cache_fail:
        VM_SET_TOP(mp_load_attr(top, CTX.qst));
        CTX.ip++;
        continue;
    }

    // This caching code works with MICROPY_PY_BUILTINS_PROPERTY and/or
    // MICROPY_PY_DESCRIPTORS enabled because if the attr exists in
    // self->members then it can't be a property or have descriptors.  A
    // consequence of this is that we can't use MP_MAP_LOOKUP_ADD_IF_NOT_FOUND
    // in the fast-path below, because that store could override a property.
    VM_ENTRY(MP_BC_STORE_ATTR): {
        MARK_EXC_IP_SELECTIVE();
        VM_DECODE_QSTR;
        mp_obj_t top = VM_TOP();
        if (mp_obj_is_instance_type(mp_obj_get_type(top)) && CTX.sp[-1] != MP_OBJ_NULL) {
            mp_obj_instance_t *self = MP_OBJ_TO_PTR(top);
            mp_uint_t x = *CTX.ip;
            mp_obj_t key = MP_OBJ_NEW_QSTR(CTX.qst);
            mp_map_elem_t *elem;
            if (x < self->members.alloc && self->members.table[x].key == key) {
                elem = &self->members.table[x];
            } else {
                elem = mp_map_lookup(&self->members, key, MP_MAP_LOOKUP);
                if (elem != NULL) {
                    *(byte*)CTX.ip = elem - &self->members.table[0];
                } else {
                    goto store_attr_cache_fail;
                }
            }
            elem->value = CTX.sp[-1];
            CTX.sp -= 2;
            CTX.ip++;
            continue;
        }
    store_attr_cache_fail:
        mp_store_attr(CTX.sp[0], CTX.qst, CTX.sp[-1]);
        CTX.sp -= 2;
        CTX.ip++;
        continue;
    }
