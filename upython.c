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



#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#else
    #define EMSCRIPTEN_KEEPALIVE
#endif

#include <stdarg.h>
#include "upython.h"


char *repl_line = NULL;

static char *stack_top;

EMSCRIPTEN_KEEPALIVE void
Py_InitializeEx(int param) {
    gc_init(heap, heap + sizeof(heap));
    mp_init();

    repl_line = (char *)malloc(REPL_INPUT_SIZE);
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
                p1 = p + strlen(p);
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



void
gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info();
}



mp_lexer_t *
mp_lexer_new_from_file(const char *filename) {
    FILE *file = fopen(filename,"r");
    if (!file)
        fprintf(stderr, "fopen failed %s\n", "filename");
    fseeko(file, 0, SEEK_END);
    off_t size_of_file = ftello(file);
    fprintf(stderr, "mp_lexer_new_from_file(%s size=%i)\n", filename, size_of_file );
    fseeko(file, 0, SEEK_SET);

    char * code_buf = malloc(size_of_file);
    fread(code_buf, size_of_file, 1, file);

    if (code_buf == NULL) {
        return NULL;
    }

    mp_lexer_t* lex = mp_lexer_new_from_str_len(qstr_from_str(filename), code_buf, strlen(code_buf), 0);
    free(code_buf);
    return lex;
}



void
do_str(const char *src, mp_parse_input_kind_t input_kind, int is_file) {
    mp_lexer_t *lex;

    if (is_file)
        lex = mp_lexer_new_from_file(src);
    else
        lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);

    if (lex == NULL) {
        printf("128:malloc: lexer\n");
        return;
    }

    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}


EMSCRIPTEN_KEEPALIVE void
PyRun_SimpleString(const char * code) {
    do_str(code, MP_PARSE_FILE_INPUT, 0);
}

EMSCRIPTEN_KEEPALIVE void
PyRun_SimpleFile(FILE *fp, const char *filename) {
    do_str(filename, MP_PARSE_FILE_INPUT, 1);
}

EMSCRIPTEN_KEEPALIVE void
PyRun_VerySimpleFile(const char *filename) {
    do_str(filename, MP_PARSE_FILE_INPUT, 1);
}

mp_import_stat_t mp_import_stat(const char *path) {
    printf("stat '%s'\n", "path");
    /*
    if (mp_js_context.import_stat == NULL) {
        return MP_IMPORT_STAT_NO_EXIST;
    }

    int s = mp_js_context.import_stat(path);

    if (s == -1) {
        return MP_IMPORT_STAT_NO_EXIST;
    } else if (s == 1) {
        return MP_IMPORT_STAT_DIR;
    } else {
        return MP_IMPORT_STAT_FILE;
    }
    */
    return MP_IMPORT_STAT_NO_EXIST;
}



int PyArg_ParseTuple(PyObject *argv, const char *fmt, ...) {
    va_list argptr;
    va_start (argptr, fmt );
    vfprintf(stderr,fmt,argptr);
    va_end (argptr);
    return 0;
}


void
nlr_jump_fail(void *val) {
    while(1);
}

void NORETURN
__fatal_error(const char *msg) {
    while (1);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
