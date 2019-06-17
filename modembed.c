#include <string.h>
#include <stdio.h>

#include "py/obj.h"
#include "py/runtime.h"

extern MPR_const_char_readline_hist

#pragma message "   ============ MOD EMBED ============="

#define None mp_const_none
#define PyObject mp_obj_t
#define bytes(cstr) PyBytes_FromString(cstr)

#define fun_obj_0(name, ...) \
    STATIC mp_obj_t embed_##name() { \
        __VA_ARGS__ \
    } \
\
    STATIC MP_DEFINE_CONST_FUN_OBJ_0(embed_##name##_obj, embed_##name);

#define def0 fun_obj_0(


STATIC mp_obj_t PyBytes_FromString(char *string){
    vstr_t vstr;
    vstr_init_len(&vstr, strlen(string));
    strcpy(vstr.buf, string);
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}


def0 os_read,{

    // simple read string
    static char buf[256];
    //fputs(p, stdout);
    char *s = fgets(buf, sizeof(buf), stdin);
    if (!s) {
        //return mp_obj_new_int(0);
        buf[0]=0;
        fprintf(stderr,"embed_os_read EOF\n" );
    } else {
        int l = strlen(buf);
        if (buf[l - 1] == '\n') {
            if ( (l>1) && (buf[l - 2] == '\r') )
                buf[l - 2] = 0;
            else
                buf[l - 1] = 0;
        } else {
            l++;
        }
        fprintf(stderr,"embed_os_read [%s]\n", buf );
    }
    return bytes(buf);
})


#define fun_obj_1(name, name0, ...) \
    STATIC mp_obj_t embed_##name(mp_obj_t arg0) { \
        ##name0 =  mp_obj_get_int(arg0); \
        __VA_ARGS__ \
    } \
\
    STATIC MP_DEFINE_CONST_FUN_OBJ_1(embed_##name##_obj, embed_##name);



STATIC mp_obj_t embed_func_3(mp_obj_t arg1, mp_obj_t arg2, mp_obj_t arg3) {
    printf("embed_func_3: arg1 = ");
    mp_obj_print(arg1, PRINT_STR);
    printf(", arg2 = ");
    mp_obj_print(arg2, PRINT_STR);
    printf(", arg3 = ");
    mp_obj_print(arg3, PRINT_STR);
    printf("\n");
    return None;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(embed_func_3_obj, embed_func_3);


/*
STATIC mp_obj_t embed_func_var(mp_uint_t n_args, const mp_obj_t *args) {
    printf("embed_func_var: num_args = %u\n", n_args);

    for (int i = 0; i < n_args; i++) {
        printf("  arg[%d] = ", i);
        mp_obj_print(args[i], PRINT_STR);
        printf("\n");
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR(embed_func_var_obj, 0, embed_func_var);
*/



STATIC mp_obj_t embed_func_str(mp_obj_t str) {
    printf("embed_func_str: str = '%s'\n", mp_obj_str_get_str(str));
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(embed_func_str_obj, embed_func_str);



STATIC mp_obj_t embed_get_history_item(mp_obj_t index) {
    printf("embed_get_history_item: '%s'\n",  MP_STATE_PORT(readline_hist)[mp_obj_get_int(index)] );

    //mp_obj_print(arg1, PRINT_STR);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(embed_get_history_item_obj, embed_get_history_item);

//     { MP _ ROM_QSTR(MP_QSTR_func_var), (mp_obj_t)&embed_func_var_obj },
STATIC const mp_map_elem_t embed_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_embed) },
    { MP_ROM_QSTR(MP_QSTR_os_read), (mp_obj_t)&embed_os_read_obj },
    { MP_ROM_QSTR(MP_QSTR_get_history_item), (mp_obj_t)&embed_get_history_item_obj },
    { MP_ROM_QSTR(MP_QSTR_func_3), (mp_obj_t)&embed_func_3_obj },
    //{ MP_ROM_QSTR(MP_QSTR_func_int), (mp_obj_t)&embed_func_int_obj },
    { MP_ROM_QSTR(MP_QSTR_func_str), (mp_obj_t)&embed_func_str_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_embed_globals, embed_globals_table);

const mp_obj_module_t mp_module_embed = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_embed_globals,
};
