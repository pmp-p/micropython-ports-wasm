

int
do_str(const char *src, mp_parse_input_kind_t input_kind) {
    int ret = 0;
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, false);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        if (mp_obj_is_subclass_fast(mp_obj_get_type((mp_obj_t)nlr.ret_val), &mp_type_SystemExit)) {
            mp_obj_t exit_val = mp_obj_exception_get_value(MP_OBJ_FROM_PTR(nlr.ret_val));
            if (exit_val != mp_const_none) {
                mp_int_t int_val;
                if (mp_obj_get_int_maybe(exit_val, &int_val)) {
                    ret = int_val & 255;
                } else {
                    ret = 1;
                }
            }
        } else {
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
            ret = 1;
        }
    }
    return 0;
}

#include "emscripten.h"

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


void
nlr_jump_fail(void *val) {
    fprintf(stderr, "FATAL: uncaught NLR %p\n", val);
    while(1);
}

#if 0//!ASYNCIFY

#if !MICROPY_READER_POSIX

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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
#endif // !MICROPY_READER_POSIX

#endif // !ASYNCIFY
