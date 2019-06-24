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

#if MICROPY_ENABLE_PYSTACK
    static mp_obj_t pystack[16384];
#endif
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



void gc_collect(void) {
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


#define IS_FILE 1
#define IS_STR 0

#if !MICROPY_VFS
// FIXME:
extern mp_import_stat_t hack_modules(const char *modname);

mp_import_stat_t mp_import_stat(const char *path) {
    fprintf(stderr,"stat(%s) : ", path);
    return hack_modules(path);
}
#endif

#if !MICROPY_READER_POSIX && MICROPY_EMIT_WASM
mp_lexer_t *
mp_lexer_new_from_file(const char *filename) {
    FILE *file = fopen(filename,"r");
    if (!file) {
        printf("404: fopen(%s)\n", filename);
        fprintf(stderr, "404: fopen(%s)\n", filename);
        return NULL;
    }
    fseeko(file, 0, SEEK_END);
    off_t size_of_file = ftello(file);
    fprintf(stderr, "mp_lexer_new_from_file(%s size=%lld)\n", filename, (long long)size_of_file );
    fseeko(file, 0, SEEK_SET);

    char * code_buf = malloc(size_of_file);
    fread(code_buf, size_of_file, 1, file);

    if (code_buf == NULL) {
        fprintf(stderr, "READ ERROR: mp_lexer_new_from_file(%s size=%lld)\n", filename, (long long)size_of_file );
        return NULL;
    }
    //fprintf(stderr, "%s" ,  code_buf);
    mp_lexer_t* lex = mp_lexer_new_from_str_len(qstr_from_str(filename), code_buf, strlen(code_buf), 0);
    free(code_buf);
    return lex;
}
#if MICROPY_HELPER_LEXER_UNIX

mp_lexer_t *mp_lexer_new_from_fd(qstr filename, int fd, bool close_fd) {
    mp_reader_t reader;
    mp_reader_new_file_from_fd(&reader, fd, close_fd);
    return mp_lexer_new(filename, reader);
}

#endif

#else
extern mp_lexer_t *mp_lexer_new_from_file(const char *filename);
#endif

void
do_code(const char *src,  int is_file) {
    mp_lexer_t *lex;
    mp_parse_input_kind_t input_kind = MP_PARSE_FILE_INPUT;

    if (is_file)
        lex = mp_lexer_new_from_file(src);
    else
        lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);

    if (lex == NULL) {
        printf("152:malloc: lexer %s\n",src);
        return;
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
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}

EMSCRIPTEN_KEEPALIVE void
PyRun_SimpleString(const char * code) {
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


void
nlr_jump_fail(void *val) {
    fprintf(stderr, "FATAL: uncaught NLR %p\n", val);
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


#if !MICROPY_READER_POSIX && MICROPY_EMIT_WASM

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "py/mperrno.h"

typedef struct _mp_reader_posix_t {
    bool close_fd;
    int fd;
    size_t len;
    size_t pos;
    byte buf[20];
} mp_reader_posix_t;

STATIC mp_uint_t mp_reader_posix_readbyte(void *data) {
    mp_reader_posix_t *reader = (mp_reader_posix_t*)data;
    if (reader->pos >= reader->len) {
        if (reader->len == 0) {
            return MP_READER_EOF;
        } else {
            int n = read(reader->fd, reader->buf, sizeof(reader->buf));
            if (n <= 0) {
                reader->len = 0;
                return MP_READER_EOF;
            }
            reader->len = n;
            reader->pos = 0;
        }
    }
    return reader->buf[reader->pos++];
}

STATIC void mp_reader_posix_close(void *data) {
    mp_reader_posix_t *reader = (mp_reader_posix_t*)data;
    if (reader->close_fd) {
        close(reader->fd);
    }
    m_del_obj(mp_reader_posix_t, reader);
}

void mp_reader_new_file_from_fd(mp_reader_t *reader, int fd, bool close_fd) {
    mp_reader_posix_t *rp = m_new_obj(mp_reader_posix_t);
    rp->close_fd = close_fd;
    rp->fd = fd;
    int n = read(rp->fd, rp->buf, sizeof(rp->buf));
    if (n == -1) {
        if (close_fd) {
            close(fd);
        }
        mp_raise_OSError(errno);
    }
    rp->len = n;
    rp->pos = 0;
    reader->data = rp;
    reader->readbyte = mp_reader_posix_readbyte;
    reader->close = mp_reader_posix_close;
}

void mp_reader_new_file(mp_reader_t *reader, const char *filename) {
    int fd = open(filename, O_RDONLY, 0644);
    if (fd < 0) {
        mp_raise_OSError(errno);
    }
    mp_reader_new_file_from_fd(reader, fd, true);
}
#endif






















































