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

#include "upython.h"


#if !MICROPY_VFS
// FIXME:
extern mp_import_stat_t
wasm_find_module(const char *modname);

mp_import_stat_t mp_import_stat(const char *path) {
    fprintf(stderr,"stat(%s) : ", path);
    return wasm_find_module(path);
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
    fseek(file, 0, SEEK_END);
    long long size_of_file = ftell(file);
    fprintf(stderr, "mp_lexer_new_from_file(%s size=%lld)\n", filename, (long long)size_of_file );
    fseek(file, 0, SEEK_SET);

    char * cbuf = malloc(size_of_file+1);
    fread(cbuf, size_of_file, 1, file);
    cbuf[size_of_file]=0;

    if (cbuf == NULL) {
        fprintf(stderr, "READ ERROR: mp_lexer_new_from_file(%s size=%lld)\n", filename, (long long)size_of_file );
        return NULL;
    }
    mp_lexer_t* lex = mp_lexer_new_from_str_len(qstr_from_str(filename), cbuf, strlen(cbuf), 0);
    //fprintf(stderr, "===== EXPECT FAILURE =====\n%s\n===== EXPECT FAILURE ========\n" ,  cbuf);
    free(cbuf); // <- remove that and emcc -shared will break
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

STATIC mp_uint_t
mp_reader_posix_readbyte(void *data) {
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

STATIC void
mp_reader_posix_close(void *data) {
    mp_reader_posix_t *reader = (mp_reader_posix_t*)data;
    if (reader->close_fd) {
        close(reader->fd);
    }
    m_del_obj(mp_reader_posix_t, reader);
}

void
mp_reader_new_file_from_fd(mp_reader_t *reader, int fd, bool close_fd) {
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

void
mp_reader_new_file(mp_reader_t *reader, const char *filename) {
    int fd = open(filename, O_RDONLY, 0644);
    if (fd < 0) {
        mp_raise_OSError(errno);
    }
    mp_reader_new_file_from_fd(reader, fd, true);
}
#endif
