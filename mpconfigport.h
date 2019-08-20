#define MODULES_LINKS

#include "dev_conf.h"

#if __DEV__
#define MICROPY_VM_HOOK_BC 1

#ifdef STATIC
    #undef STATIC
#endif

#define STATIC    // mpconfig.h:1402   => bad pointer cast and bad linking
#endif




#define mp_hal_ticks_cpu() 0


// options to control how Micro Python is built
#define MICROPY_QSTR_BYTES_IN_HASH  (1)
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_ENABLE_EMERGENCY_EXCEPTION_BUF (1)
#define MICROPY_CPYTHON_COMPAT      (1)
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_DOUBLE)
//just in case of long urls and a local cache ?
#define MICROPY_ALLOC_PATH_MAX      (1024)
#define MICROPY_BUILTIN_METHOD_CHECK_SELF_ARG (0)
#define MICROPY_CAN_OVERRIDE_BUILTINS (1)

// 1 or build/frozen_mpy.c:7:2: error: "incompatible MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE"
#define MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE (1)

#define MICROPY_MODULE_WEAK_LINKS   (0)

#define MICROPY_MODULE_GETATTR (1)
#define MICROPY_MODULE_SPECIAL_METHODS (1)


#define MICROPY_QSTR_EXTRA_POOL     mp_qstr_frozen_const_pool

#define MICROPY_PY_BUILTINS_FROZENSET (1)

#define MICROPY_LONGINT_IMPL_MPZ    (2)
#define MICROPY_LONGINT_IMPL        (MICROPY_LONGINT_IMPL_MPZ)
#define MPZ_DIG_SIZE                (16)


#define MICROPY_ENABLE_COMPILER     (1)
#define MICROPY_ENABLE_EXTERNAL_IMPORT (1)
#define MICROPY_PY_BUILTINS_COMPILE (1)
#define MICROPY_PY_SYS_EXC_INFO     (0) // <============================ NON STANDARD PY2

#define MICROPY_PERSISTENT_CODE (1)
#define MICROPY_PERSISTENT_CODE_LOAD (1)
#define MICROPY_PERSISTENT_CODE_SAVE (1)
//#define MICROPY_EMIT_MACHINE_CODE (1)


#define MICROPY_HAS_FILE_READER (1)
#define MICROPY_HELPER_LEXER_UNIX (1)

#define MICROPY_READER_POSIX (0)
#define MICROPY_VFS_POSIX    (0)

#define HAVE_mp_raw_code_save_file (1)

#define MICROPY_EMIT_WASM (1)

//MICROPY_EMIT_INLINE_ASM ((MICROPY_EMIT_INLINE_THUMB || MICROPY_EMIT_INLINE_XTENSA)
//#define MICROPY_EMIT_NATIVE

#define MICROPY_EMIT_X64            (0)
#define MICROPY_EMIT_THUMB          (0)
#define MICROPY_EMIT_INLINE_THUMB   (0)


#if 1
// 0 0 0 0 ! RuntimeError: memory access out of bounds
// 0 0 0 1 ! silent crash
// 0 0 1 0 = 11.15 Kpy/s
    #define MICROPY_STACKLESS           (0)
    #define MICROPY_STACKLESS_STRICT    (0)
    #define MICROPY_ENABLE_PYSTACK      (1)
    #define MICROPY_OPT_COMPUTED_GOTO   (0)
#else
// 1 0 1 1  = 10.6 Kpy/s
// 1 0 1 0  = 10.8 K
// 1 1 1 0  = 10.6
// 1 1 1 1  = 10.6
    #define MICROPY_STACKLESS           (1)
    #define MICROPY_STACKLESS_STRICT    (1)   // <=============== 1! or too much collect
    #define MICROPY_ENABLE_PYSTACK      (1)
    #define MICROPY_OPT_COMPUTED_GOTO   (0)
#endif


// nlr.h  MICROPY_NLR_* must match a supported arch
// define or autodetect will fail to select WASM
#define MICROPY_NLR_SETJMP          (1)
#define MICROPY_DYNAMIC_COMPILER    (0)

#if MICROPY_NLR_SETJMP
    // can't have MICROPY_NLR_SETJMP==MICROPY_DYNAMIC_COMPILER==0
    #define MICROPY_NLR_OS_WINDOWS (0)
    #define MICROPY_NLR_X86 (0)
    #define MICROPY_NLR_X64 (0)
#endif

// mpconfig
#define MICROPY_PY_GENERATOR_PEND_THROW (1)  // def 1
#define MICROPY_PY_STR_BYTES_CMP_WARN (1)    // def 0


// MEMORY
#define MICROPY_PY_MICROPYTHON_MEM_INFO   (1)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_GC_ALLOC_THRESHOLD  (1)

//like ESP/UNIX
#define MICROPY_ALLOC_PARSE_CHUNK_INIT (64)


#if MICROPY_PY_LVGL
#define MICROPY_MEM_STATS           (0)
#else
#define MICROPY_MEM_STATS           (1)
#endif

#if MICROPY_MEM_STATS
#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)
#endif


#define MICROPY_KBD_EXCEPTION       (1)


//??
#define MICROPY_USE_INTERNAL_ERRNO (0)

#define MICROPY_USE_READLINE (0)
#define MICROPY_PY_BUILTINS_INPUT (1)


#define MICROPY_PY_COLLECTIONS_ORDEREDDICT (1)
#define MICROPY_PY_DELATTR_SETATTR  (1)
#define MICROPY_PY_MATH_SPECIAL_FUNCTIONS (1)
#define MICROPY_PY_MATH_FACTORIAL   (1)


#define MICROPY_COMP_MODULE_CONST   (1)
#define MICROPY_COMP_CONST          (1)
#define MICROPY_COMP_DOUBLE_TUPLE_ASSIGN (1)
#define MICROPY_COMP_TRIPLE_TUPLE_ASSIGN (1)

#define MICROPY_ENABLE_SOURCE_LINE  (1)

#define MICROPY_HELPER_REPL         (1)

#define MICROPY_PY___FILE__         (1)

#define MICROPY_PY_ALL_SPECIAL_METHODS (1)

#define MICROPY_PY_ARRAY            (1)
#define MICROPY_PY_ASYNC_AWAIT      (1)
#define MICROPY_PY_ATTRTUPLE        (1)

#define MICROPY_PY_BTREE            (0)

#define MICROPY_PY_BUILTINS_BYTEARRAY (1)
#define MICROPY_PY_DESCRIPTORS        (1)
#define MICROPY_PY_BUILTINS_ENUMERATE (1)
#define MICROPY_PY_BUILTINS_EXECFILE  (1) // <============================ NON STANDARD PY2
#define MICROPY_PY_BUILTINS_FILTER    (1)
//#define MICROPY_PY_BUILTINS_FLOAT     (1) because MICROPY_FLOAT_IMPL_DOUBLE
#define MICROPY_PY_BUILTINS_HELP_MODULES (0)
#define MICROPY_PY_BUILTINS_MEMORYVIEW (1)
#define MICROPY_PY_BUILTINS_MIN_MAX   (1)
#define MICROPY_PY_BUILTINS_NEXT2     (1) // <=============== next2 make next() compat with cpython
#define MICROPY_PY_BUILTINS_PROPERTY  (1)
#define MICROPY_PY_BUILTINS_REVERSED  (1)
#define MICROPY_PY_BUILTINS_SET       (1)
#define MICROPY_PY_BUILTINS_SLICE     (1)
#define MICROPY_PY_BUILTINS_STR_UNICODE (0)
#define MICROPY_PY_BUILTINS_STR_CENTER (1)
#define MICROPY_PY_BUILTINS_STR_PARTITION (1)
#define MICROPY_PY_BUILTINS_STR_SPLITLINES (1)
#define MICROPY_PY_BUILTINS_SLICE_ATTRS (1)

#define MICROPY_PY_COLLECTIONS      (1)
#define MICROPY_PY_CMATH            (1)

//? TEST THAT THING !
#if 1
    #define MICROPY_PY_FFI              (0)
#else
#define MICROPY_PY_FFI              (1) // <========================== NON STANDARD NOT CPY
#endif

#define MICROPY_PY_FUNCTION_ATTRS   (1)
#define MICROPY_PY_GC               (1)
#define MICROPY_PY_MATH             (1)
#define MICROPY_PY_STRUCT           (1)
#define MICROPY_PY_SYS              (1)
#define MICROPY_PY_SYS_GETSIZEOF    (1)
#define MICROPY_PY_SYS_MODULES      (1)
#define MICROPY_PY_SYS_MAXSIZE      (1)
#define MICROPY_PY_SYS_PLATFORM     "wasm"

// IO is cooked and multiplexed via mp_hal_stdout_tx_strn in file.c
// so do not modify those
// https://github.com/python/cpython/blob/v3.7.3/Python/bltinmodule.c#L1849-L1932


#define MICROPY_USE_INTERNAL_PRINTF (0)
#define MICROPY_PY_IO               (1) // <=============== only 1 for wasm port
#define MICROPY_PY_IO_IOBASE            (1)
#define MICROPY_PY_IO_RESOURCE_STREAM   (1)
#define MICROPY_PY_IO_FILEIO            (1)
#define MICROPY_PY_IO_BYTESIO           (1)
#define MICROPY_PY_IO_BUFFEREDWRITER    (1)

#define MICROPY_PY_SYS_STDIO_BUFFER (1)
#define MICROPY_PY_SYS_STDFILES     (1)
#define MICROPY_PY_OS_DUPTERM       (1) // <==== or js console will get C stdout too ( C-OUT [spam] )


#define MICROPY_PY_THREAD           (0)
#define MICROPY_PY_THREAD_GIL       (0)
#define MICROPY_PY_TIME             (0)

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


//TODO: need #define MICROPY_EVENT_POLL_HOOK + select_select on io demultiplexer.
//F
#define MICROPY_PY_USELECT          (0)


#define MICROPY_PY_UTIME            (1)
#define MICROPY_PY_UTIME_MP_HAL     (1)
#define MICROPY_PY_UTIMEQ           (1)
#define MICROPY_PY_UZLIB            (1)

#define MICROPY_VFS                 (0)

#define MICROPY_DEBUG_PRINTERS      (0)
#define MICROPY_MPY_CODE_SAVE       (1)
#define MICROPY_REPL_EVENT_DRIVEN   (1)
#define MICROPY_ENABLE_DOC_STRING   (1)


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
//#define MP_PLAT_PRINT_STRN(str, len) mp_hal_stdout_tx_strn(str, len)


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


#if MICROPY_PY_LVGL
    extern const struct _mp_obj_module_t mp_module_utime;
    extern const struct _mp_obj_module_t mp_module_lvgl;
    extern const struct _mp_obj_module_t mp_module_lvindev;
    extern const struct _mp_obj_module_t mp_module_SDL;

    #include "lib/lv_bindings/lvgl/src/lv_misc/lv_gc.h"

    #define MICROPY_PY_LVGL_DEF \
        { MP_OBJ_NEW_QSTR(MP_QSTR_lvgl), (mp_obj_t)&mp_module_lvgl },\
        { MP_OBJ_NEW_QSTR(MP_QSTR_lvindev), (mp_obj_t)&mp_module_lvindev},\
        { MP_OBJ_NEW_QSTR(MP_QSTR_SDL), (mp_obj_t)&mp_module_SDL },

    #define MPR_void_mp_lv_user_data void *mp_lv_user_data;
    #define MPR_LVGL LV_GC_ROOTS(LV_NO_PREFIX)
#else
    #define MICROPY_PY_LVGL_DEF
    #define MPR_void_mp_lv_user_data
    #define MPR_LVGL
#endif


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
    MICROPY_PY_LVGL_DEF \
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
#define MICROPY_HW_MCU_NAME "wasm"

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
 MPR_LVGL \
 MPR_void_mp_lv_user_data \
 MPR_const_char_readline_hist \
 void *PyOS_InputHook; \
 int coro_call_counter; \


#define FFCONF_H "lib/oofatfs/ffconf.h"








//
