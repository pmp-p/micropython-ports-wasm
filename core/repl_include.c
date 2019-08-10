#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "py/nlr.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "lib/utils/pyexec.h"
#include "py/mphal.h"


#include <stdarg.h>
#include "upython.h"

#include <mpconfigport.h>

EMSCRIPTEN_KEEPALIVE char *repl_line = NULL;

EMSCRIPTEN_KEEPALIVE static char *stack_top;

#if MICROPY_ENABLE_PYSTACK
    static mp_obj_t pystack[16384];
#endif

void clear_shared_array_buffer() {
    for (int i=0;i<REPL_INPUT_SIZE;i++)
        repl_line[i] = 0;
}

EMSCRIPTEN_KEEPALIVE void
Py_InitializeEx(int param) {

    #if MICROPY_ENABLE_GC
        gc_init(heap, heap + sizeof(heap));
    #endif

    #if MICROPY_ENABLE_PYSTACK
    // thx @embeddedt
    // https://github.com/littlevgl/lv_micropython/commit/19b2bec8863924c8a46f7761c4568dea2873670d
        mp_pystack_init(pystack, &pystack[MP_ARRAY_SIZE(pystack)]);
    #endif

    mp_init();

    repl_line = (char *)malloc(REPL_INPUT_SIZE);
    // this is for null end str !!!!!
    clear_shared_array_buffer();

    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), 0);

    char *home = getenv("HOME");
    char *path = getenv("MICROPYPATH");

    if (path == NULL) {
        #ifdef MICROPY_PY_SYS_PATH_DEFAULT
            path = MICROPY_PY_SYS_PATH_DEFAULT;
        #else
            path = "~/.micropython/lib:/usr/lib/micropython";
        #endif
    }
    size_t path_num = 1; // [0] is for current dir (or base dir of the script)
    for (char *p = path; p != NULL; p = strchr(p, PATHLIST_SEP_CHAR)) {
        path_num++;
        if (p != NULL) {
            p++;
        }
    }
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), path_num);
    mp_obj_t *path_items;
    mp_obj_list_get(mp_sys_path, &path_num, &path_items);
    path_items[0] = MP_OBJ_NEW_QSTR(MP_QSTR_);

    {
        char *p = path;
        for (mp_uint_t i = 1; i < path_num; i++) {
            char *p1 = strchr(p, PATHLIST_SEP_CHAR);
            if (p1 == NULL) {
                p1 = p + bsd_strlen(p);
            }
            if (p[0] == '~' && p[1] == '/' && home != NULL) {
                // Expand standalone ~ to $HOME
                int home_l = strlen(home);
                vstr_t vstr;
                vstr_init(&vstr, home_l + (p1 - p - 1) + 1);
                vstr_add_strn(&vstr, home, home_l);
                vstr_add_strn(&vstr, p + 1, p1 - p - 1);
                path_items[i] = mp_obj_new_str_from_vstr(&mp_type_str, &vstr);
            } else {
                path_items[i] = MP_OBJ_NEW_QSTR(qstr_from_strn(p, p1 - p));
            }
            p = p1 + 1;
        }
    }



}



EMSCRIPTEN_KEEPALIVE void
gc_collect(void) {
#if MICROPY_ENABLE_GC
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    jmp_buf dummy;
    if (setjmp(dummy) == 0) {
        longjmp(dummy, 1);
    }
    gc_collect_start();
    gc_collect_root((void*)stack_top, ((mp_uint_t)(void*)(&dummy + 1) - (mp_uint_t)stack_top) / sizeof(mp_uint_t));
    gc_collect_end();
#endif
}



EMSCRIPTEN_KEEPALIVE int
do_code(const char *src,  int is_file) {
    mp_lexer_t *lex;
    mp_parse_input_kind_t input_kind = MP_PARSE_FILE_INPUT;

    //puts("do_code");
    if (is_file)
        lex = mp_lexer_new_from_file(src);
    else
        lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, bsd_strlen(src), 0);

    if (lex == NULL) {
        printf("152:malloc: lexer %s\n",src);
        return 0;
    }

    qstr source_name = lex->source_name;

    #if MICROPY_PY___FILE__
    if (input_kind == MP_PARSE_FILE_INPUT) {
        mp_store_global(MP_QSTR___file__, MP_OBJ_NEW_QSTR(source_name));
    }
    #endif

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
        return 1;
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        return 0;
    }
}

EMSCRIPTEN_KEEPALIVE void
PyRun_SimpleString(const char * code) {
    //puts("PyRun_SimpleString");
    do_code(code,  IS_STR);
}

EMSCRIPTEN_KEEPALIVE void
PyRun_SimpleFile(FILE *fp, const char *filename) {
    do_code(filename,  IS_FILE);
}

EMSCRIPTEN_KEEPALIVE void
PyRun_VerySimpleFile(const char *filename) {
    do_code(filename, IS_FILE);
}


int PyArg_ParseTuple(PyObject *argv, const char *fmt, ...) {
    va_list argptr;
    va_start (argptr, fmt );
    vfprintf(stderr,fmt,argptr);
    va_end (argptr);
    return 0;
}


EMSCRIPTEN_KEEPALIVE void
nlr_jump_fail(void *val) {
    fprintf(stderr, "FATAL: uncaught NLR %p\n", val);
    while(1);
}

EMSCRIPTEN_KEEPALIVE void NORETURN
__fatal_error(const char *msg) {
    while (1);
}

#ifndef NDEBUG
void MP_WEAK
__assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
