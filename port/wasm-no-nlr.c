void
CLOG(const char str[]) {
    mp_js_write(&str[0],strlen(&str[0]));
}

int
do_str(const char *src, mp_parse_input_kind_t input_kind) {
    int retval = 0;
    mp_obj_t ret = MP_OBJ_NULL;
    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
    qstr source_name = lex->source_name;
    mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
    mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);

    if (module_fun != MP_OBJ_NULL) {
        //CLOG("hop");
        ret = mp_call_function_0(module_fun);
        if ((ret != MP_OBJ_NULL) && (MP_STATE_VM(mp_pending_exception) != MP_OBJ_NULL)) {
            CLOG("Exception!");
            mp_obj_t obj = MP_STATE_VM(mp_pending_exception);

            MP_STATE_VM(mp_pending_exception) = MP_OBJ_NULL;
            mp_obj_print_exception(&mp_plat_print, obj);
            mp_raise_o(obj);

            retval = 1;
        }
        //else CLOG("OK!");

        //CLOG("bof");
    }

    return retval;
}




NORETURN void nlr_jump(void *val) {
    while (1){
    fprintf(stderr,"30: WASx:nlr_jump\n");
    }
}

void
gc_collect(void) {
    //fprintf(stderr,"36: WASx:gc_collect\n");
}



#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/reader.h"

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
    if (rp == NULL) {
        return;
    }
    rp->close_fd = close_fd;
    rp->fd = fd;
    int n = read(rp->fd, rp->buf, sizeof(rp->buf));
    if (n == -1) {
        if (close_fd) {
            close(fd);
        }
        mp_raise_OSError_o(errno);
        return;
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
        mp_raise_OSError_o(errno);
        return;
    }
    mp_reader_new_file_from_fd(reader, fd, true);
}

#endif // !MICROPY_READER_POSIX
