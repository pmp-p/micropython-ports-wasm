#include <stdint.h>

#define MODULES_H "modules.h"
#include MODULES_H

// options to control how Micro Python is built

#define MICROPY_QSTR_BYTES_IN_HASH  (1)
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_TERSE)
#define MICROPY_CPYTHON_COMPAT      (1)
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_DOUBLE)
#define MICROPY_ALLOC_PATH_MAX      (256)
#define MICROPY_ALLOC_PARSE_CHUNK_INIT (16)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_CAN_OVERRIDE_BUILTINS (1)

//FREEZING MODULES
#define MICROPY_MODULE_FROZEN       (1)
#define MICROPY_MODULE_FROZEN_MPY   (1)
#define MICROPY_MODULE_FROZEN_STR   (1)
#define FROZEN_MPY_DIR              "modules"
//set by Makefile
#define MICROPY_QSTR_EXTRA_POOL     mp_qstr_frozen_const_pool

#define MICROPY_PY_BUILTINS_FROZENSET (1)
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (1)
#define MICROPY_LONGINT_IMPL_MPZ    (2)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_MPZ)
#define MPZ_DIG_SIZE                (16)


#define MICROPY_ENABLE_COMPILER     (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO (1)


#define MICROPY_NLR_SETJMP          (1)   //nlr.h  MICROPY_NLR_* must match a supported arch



//??
#define MICROPY_USE_INTERNAL_ERRNO (0)
#define MICROPY_USE_INTERNAL_PRINTF (0)

#define MICROPY_PY_DELATTR_SETATTR (0)
#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (0)
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS (0)
#define MICROPY_PY_MATH_FACTORIAL   (0)
#define MICROPY_PY_SYS_GETSIZEOF    (0)



#define MICROPY_COMP_MODULE_CONST   (1)
#define MICROPY_COMP_CONST          (1)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (1)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_ENABLE_SOURCE_LINE  (1)


#define MICROPY_MODULE_GETATTR (1)
#define MICROPY_MODULE_SPECIAL_METHODS (1)

#define MICROPY_PY_BUILTINS_BYTEARRAY (1)
#define MICROPY_PY_BUILTINS_ENUMERATE (1)
#define MICROPY_PY_BUILTINS_EXECFILE  (1)
#define MICROPY_PY_BUILTINS_FILTER    (1)
//#define MICROPY_PY_BUILTINS_FLOAT     (1) because MICROPY_FLOAT_IMPL_DOUBLE == MICROPY_FLOAT_IMPL_DOUBLE
#define MICROPY_PY_BUILTINS_MEMORYVIEW (1)
#define MICROPY_PY_BUILTINS_MIN_MAX   (1)
#define MICROPY_PY_BUILTINS_PROPERTY  (1)
#define MICROPY_PY_BUILTINS_REVERSED  (1)
#define MICROPY_PY_BUILTINS_SET       (1)
#define MICROPY_PY_BUILTINS_SLICE     (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE (1)


#define MICROPY_PY_ASYNC_AWAIT      (1)
#define MICROPY_PY_ATTRTUPLE        (1)
#define MICROPY_PY_BTREE            (0)
#define MICROPY_PY_COLLECTIONS      (1)
#define MICROPY_PY_CMATH            (1)
//?
#define MICROPY_PY_FFI              (1)
#define MICROPY_PY_FUNCTION_ATTRS   (1)
#define MICROPY_PY_GC               (1)
#define MICROPY_PY_MATH             (1)
#define MICROPY_PY_STRUCT           (1)
#define MICROPY_PY_SYS              (1)
#define MICROPY_PY_SYS_PLATFORM     "wasm"
#define MICROPY_PY_SYS_MAXSIZE      (1)

#define MICROPY_PY_ARRAY            (1)
#define MICROPY_PY_UBINASCII        (1)
#define MICROPY_PY_UCTYPES          (1)
#define MICROPY_PY_UERRNO           (1)
#define MICROPY_PY_UERRNO_ERRORCODE (1)
#define MICROPY_PY_UHEAPQ           (1)
#define MICROPY_PY_UHASHLIB         (1)
//F
#define MICROPY_PY_UHASHLIB_SHA1    (0)
#define MICROPY_PY_UHASHLIB_SHA256  (1)
#define MICROPY_PY_UJSON            (1)
#define MICROPY_PY_UOS              (1)
#define MICROPY_PY_URANDOM          (1)
#define MICROPY_PY_URANDOM_EXTRA_FUNCS (1)
#define MICROPY_PY_URE              (1)
#define MICROPY_PY_URE_SUB          (1)
//F
#define MICROPY_PY_USELECT          (0) //? need #define MICROPY_EVENT_POLL_HOOK + select_select
#define MICROPY_PY_UTIME            (1)
#define MICROPY_PY_TIME             (0)
#define MICROPY_PY_UTIMEQ           (1)
#define MICROPY_PY_UZLIB            (1)

#define MICROPY_PY_IO               (1)
#define MICROPY_VFS                 (0)
#define MICROPY_PY_IO_IOBASE            (1)
#define MICROPY_PY_IO_RESOURCE_STREAM   (1)
#define MICROPY_PY_IO_FILEIO            (1)
#define MICROPY_PY_IO_BYTESIO           (1)
#define MICROPY_PY_IO_BUFFEREDWRITER    (1)

#define MICROPY_PY___FILE__         (0)
#define MICROPY_DEBUG_PRINTERS      (0)
#define MICROPY_MEM_STATS           (0)
#define MICROPY_MPY_CODE_SAVE       (0)
#define MICROPY_GC_ALLOC_THRESHOLD  (0)
#define MICROPY_REPL_EVENT_DRIVEN   (0)
#define MICROPY_HELPER_LEXER_UNIX   (0)
#define MICROPY_ENABLE_DOC_STRING   (0)
#define MICROPY_EMIT_X64            (0)
#define MICROPY_EMIT_THUMB          (0)
#define MICROPY_EMIT_INLINE_THUMB   (0)

//asyncio implemented
#define MICROPY_ENABLE_SCHEDULER (0)
#define MICROPY_SCHEDULER_DEPTH  (4)


// type definitions for the specific machine

#define BYTES_PER_WORD (4)

#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void*)((mp_uint_t)(p) | 1))

// This port is intended to be 32-bit, but unfortunately, int32_t for
// different targets may be defined in different ways - either as int
// or as long. This requires different printf formatting specifiers
// to print such value. So, we avoid int32_t and use int directly.
#define UINT_FMT "%u"
#define INT_FMT "%d"
typedef int mp_int_t; // must be pointer size
typedef unsigned mp_uint_t; // must be pointer size

typedef long mp_off_t;

#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn_cooked(str, len)


extern const struct _mp_obj_module_t mp_module_time;
extern const struct _mp_obj_module_t mp_module_io;
extern const struct _mp_obj_module_t mp_module_os;

#if MICROPY_PY_UOS_VFS
    #define MICROPY_PY_UOS_DEF { MP_ROM_QSTR(MP_QSTR_uos), MP_ROM_PTR(&mp_module_uos_vfs) },
#else
    #define MICROPY_PY_UOS_DEF { MP_ROM_QSTR(MP_QSTR_uos), (mp_obj_t)&mp_module_os },
#endif

#if MICROPY_PY_FFI
    extern const struct _mp_obj_module_t mp_module_ffi;
    #define MICROPY_PY_FFI_DEF { MP_ROM_QSTR(MP_QSTR_ffi), MP_ROM_PTR(&mp_module_ffi) },
#else
    #define MICROPY_PY_FFI_DEF
#endif

#if MICROPY_PY_JNI
    #define MICROPY_PY_JNI_DEF { MP_ROM_QSTR(MP_QSTR_jni), MP_ROM_PTR(&mp_module_jni) },
#else
    #define MICROPY_PY_JNI_DEF
#endif

#if MICROPY_PY_TERMIOS
    #define MICROPY_PY_TERMIOS_DEF { MP_ROM_QSTR(MP_QSTR_termios), MP_ROM_PTR(&mp_module_termios) },
#else
    #define MICROPY_PY_TERMIOS_DEF
#endif

#if MICROPY_PY_SOCKET
    #define MICROPY_PY_SOCKET_DEF { MP_ROM_QSTR(MP_QSTR_usocket), MP_ROM_PTR(&mp_module_socket) },
#else
    #define MICROPY_PY_SOCKET_DEF
#endif

#if MICROPY_PY_USELECT_POSIX
    #define MICROPY_PY_USELECT_DEF { MP_ROM_QSTR(MP_QSTR_uselect), MP_ROM_PTR(&mp_module_uselect) },
#else
    #define MICROPY_PY_USELECT_DEF
#endif

#define MICROPY_PY_MACHINE_DEF

// { MP _ ROM_QSTR(MP_QSTR_umachine), MP _ ROM_PTR(&mp_module_machine) },

#define MICROPY_PORT_BUILTIN_MODULES \
    { MP_OBJ_NEW_QSTR(MP_QSTR_utime), (mp_obj_t)&mp_module_time }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_io), (mp_obj_t)&mp_module_io }, \
    MICROPY_PY_FFI_DEF \
    MICROPY_PY_JNI_DEF \
    MICROPY_PY_SOCKET_DEF \
    MICROPY_PY_MACHINE_DEF \
    MICROPY_PY_UOS_DEF \
    MICROPY_PY_USELECT_DEF \
    MICROPY_PY_TERMIOS_DEF \
    MODULES_LINKS



// extra built in names to add to the global namespace
#define MICROPY_PORT_BUILTINS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_open), MP_ROM_PTR(&mp_builtin_open_obj) }, \



#define MICROPY_PORT_BUILTIN_MODULE_WEAK_LINKS \
    { MP_OBJ_NEW_QSTR(MP_QSTR_binascii), (mp_obj_t)&mp_module_ubinascii }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_collections), (mp_obj_t)&mp_module_collections }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_errno), (mp_obj_t)&mp_module_uerrno }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_hashlib), (mp_obj_t)&mp_module_uhashlib }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_heapq), (mp_obj_t)&mp_module_uheapq }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_io), (mp_obj_t)&mp_module_io }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_json), (mp_obj_t)&mp_module_ujson }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_os), (mp_obj_t)&mp_module_os }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_random), (mp_obj_t)&mp_module_urandom }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_re), (mp_obj_t)&mp_module_ure }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_select), (mp_obj_t)&mp_module_uselect }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_socket), (mp_obj_t)&mp_module_usocket }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_ssl), (mp_obj_t)&mp_module_ussl }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_struct), (mp_obj_t)&mp_module_ustruct }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_time), (mp_obj_t)&mp_module_time }, \
    { MP_OBJ_NEW_QSTR(MP_QSTR_zlib), (mp_obj_t)&mp_module_uzlib }, \





// We need to provide a declaration/definition of alloca()
#include <alloca.h>

#define MICROPY_HW_BOARD_NAME "emscripten"
#define MICROPY_HW_MCU_NAME "asmjs"

#ifdef __linux__
#define MICROPY_MIN_USE_STDOUT (1)
#endif

#ifdef __thumb__
#define MICROPY_MIN_USE_CORTEX_CPU (1)
#define MICROPY_MIN_USE_STM32_MCU (1)
#endif

#define MP_STATE_PORT MP_STATE_VM

#define MPR_const_char_readline_hist const char *readline_hist[16];

#define MICROPY_PORT_ROOT_POINTERS \
 MPR_const_char_readline_hist






#define FFCONF_H "lib/oofatfs/ffconf.h"








//
