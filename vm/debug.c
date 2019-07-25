#include <stdio.h>
#include <assert.h>

#include "py/bc0.h"
#include "py/bc.h"


#define logd(...) fprintf(stderr, __VA_ARGS__ )

#include "py/bc0.h"
#include "py/bc.h"

#define VM_DECODERS 1

#define DBG_DECODE_UINT { \
    unum = 0; \
    do { \
        unum = (unum << 7) + (*ip & 0x7f); \
    } while ((*ip++ & 0x80) != 0); \
}
#define DBG_DECODE_ULABEL do { unum = (ip[0] | (ip[1] << 8)); ip += 2; } while (0)
#define DBG_DECODE_SLABEL do { unum = (ip[0] | (ip[1] << 8)) - 0x8000; ip += 2; } while (0)

#define DBG_DECODE_QSTR \
    qst = ip[0] | ip[1] << 8; \
    ip += 2;
#define DBG_DECODE_PTR \
    DBG_DECODE_UINT; \
    unum = mp_showbc_const_table[unum]
#define DBG_DECODE_OBJ \
    DBG_DECODE_UINT; \
    unum = mp_showbc_const_table[unum]


const byte *mp_showbc_code_start;
const mp_uint_t *mp_showbc_const_table;

void mp_bytecode_print(const void *descr, const byte *ip, mp_uint_t len, const mp_uint_t *const_table) {
    mp_showbc_code_start = ip;

    // get bytecode parameters
    mp_uint_t n_state = mp_decode_uint(&ip);
    mp_uint_t n_exc_stack = mp_decode_uint(&ip);
    /*mp_uint_t scope_flags =*/ ip++;
    mp_uint_t n_pos_args = *ip++;
    mp_uint_t n_kwonly_args = *ip++;
    /*mp_uint_t n_def_pos_args =*/ ip++;

    const byte *code_info = ip;
    mp_uint_t code_info_size = mp_decode_uint(&code_info);
    ip += code_info_size;

    #if MICROPY_PERSISTENT_CODE
    qstr block_name = code_info[0] | (code_info[1] << 8);
    qstr source_file = code_info[2] | (code_info[3] << 8);
    code_info += 4;
    #else
    qstr block_name = mp_decode_uint(&code_info);
    qstr source_file = mp_decode_uint(&code_info);
    #endif
    logd("File %s, code block '%s' (descriptor: %p, bytecode @%p " UINT_FMT " bytes)\n",
        qstr_str(source_file), qstr_str(block_name), descr, mp_showbc_code_start, len);

    // raw bytecode dump
    logd("Raw bytecode (code_info_size=" UINT_FMT ", bytecode_size=" UINT_FMT "):\n", code_info_size, len - code_info_size);
    for (mp_uint_t i = 0; i < len; i++) {
        if (i > 0 && i % 16 == 0) {
            logd("\n");
        }
        logd(" %02x", mp_showbc_code_start[i]);
    }
    logd("\n");

    // bytecode prelude: arg names (as qstr objects)
    logd("arg names:");
    for (mp_uint_t i = 0; i < n_pos_args + n_kwonly_args; i++) {
        logd(" %s", qstr_str(MP_OBJ_QSTR_VALUE(const_table[i])));
    }
    logd("\n");

    logd("(N_STATE " UINT_FMT ")\n", n_state);
    logd("(N_EXC_STACK " UINT_FMT ")\n", n_exc_stack);

    // for printing line number info
    const byte *bytecode_start = ip;

    // bytecode prelude: initialise closed over variables
    {
        uint local_num;
        while ((local_num = *ip++) != 255) {
            logd("(INIT_CELL %u)\n", local_num);
        }
        len -= ip - mp_showbc_code_start;
    }

    // print out line number info
    {
        mp_int_t bc = bytecode_start - ip;
        mp_uint_t source_line = 1;
        logd("  bc=" INT_FMT " line=" UINT_FMT "\n", bc, source_line);
        for (const byte* ci = code_info; *ci;) {
            if ((ci[0] & 0x80) == 0) {
                // 0b0LLBBBBB encoding
                bc += ci[0] & 0x1f;
                source_line += ci[0] >> 5;
                ci += 1;
            } else {
                // 0b1LLLBBBB 0bLLLLLLLL encoding (l's LSB in second byte)
                bc += ci[0] & 0xf;
                source_line += ((ci[0] << 4) & 0x700) | ci[1];
                ci += 2;
            }
            logd("  bc=" INT_FMT " line=" UINT_FMT "\n", bc, source_line);
        }
    }
    mp_bytecode_print2(ip, len - 0, const_table);
}

const byte *mp_bytecode_print_str(const byte *ip) {
    mp_uint_t unum;
    qstr qst;

    switch (*ip++) {
        case MP_BC_LOAD_CONST_FALSE:
            logd("LOAD_CONST_FALSE");
            break;

        case MP_BC_LOAD_CONST_NONE:
            logd("LOAD_CONST_NONE");
            break;

        case MP_BC_LOAD_CONST_TRUE:
            logd("LOAD_CONST_TRUE");
            break;

        case MP_BC_LOAD_CONST_SMALL_INT: {
            mp_int_t num = 0;
            if ((ip[0] & 0x40) != 0) {
                // Number is negative
                num--;
            }
            do {
                num = (num << 7) | (*ip & 0x7f);
            } while ((*ip++ & 0x80) != 0);
            logd("LOAD_CONST_SMALL_INT " INT_FMT, num);
            break;
        }

        case MP_BC_LOAD_CONST_STRING:
            DBG_DECODE_QSTR;
            logd("LOAD_CONST_STRING '%s'", qstr_str(qst));
            break;

        case MP_BC_LOAD_CONST_OBJ:
            DBG_DECODE_OBJ;
            logd("LOAD_CONST_OBJ %p=", MP_OBJ_TO_PTR(unum));
            mp_obj_print_helper(&mp_plat_print, (mp_obj_t)unum, PRINT_REPR);
            break;

        case MP_BC_LOAD_NULL:
            logd("LOAD_NULL");
            break;

        case MP_BC_LOAD_FAST_N:
            DBG_DECODE_UINT;
            logd("LOAD_FAST_N " UINT_FMT, unum);
            break;

        case MP_BC_LOAD_DEREF:
            DBG_DECODE_UINT;
            logd("LOAD_DEREF " UINT_FMT, unum);
            break;

        case MP_BC_LOAD_NAME:
            DBG_DECODE_QSTR;
            logd("LOAD_NAME %s", qstr_str(qst));
            if (MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE) {
                logd(" (cache=%u)", *ip++);
            }
            break;

        case MP_BC_LOAD_GLOBAL:
            DBG_DECODE_QSTR;
            logd("LOAD_GLOBAL %s", qstr_str(qst));
            if (MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE) {
                logd(" (cache=%u)", *ip++);
            }
            break;

        case MP_BC_LOAD_ATTR:
            DBG_DECODE_QSTR;
            logd("LOAD_ATTR %s", qstr_str(qst));
            if (MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE) {
                logd(" (cache=%u)", *ip++);
            }
            break;

        case MP_BC_LOAD_METHOD:
            DBG_DECODE_QSTR;
            logd("LOAD_METHOD %s", qstr_str(qst));
            break;

        case MP_BC_LOAD_SUPER_METHOD:
            DBG_DECODE_QSTR;
            logd("LOAD_SUPER_METHOD %s", qstr_str(qst));
            break;

        case MP_BC_LOAD_BUILD_CLASS:
            logd("LOAD_BUILD_CLASS");
            break;

        case MP_BC_LOAD_SUBSCR:
            logd("LOAD_SUBSCR");
            break;

        case MP_BC_STORE_FAST_N:
            DBG_DECODE_UINT;
            logd("STORE_FAST_N " UINT_FMT, unum);
            break;

        case MP_BC_STORE_DEREF:
            DBG_DECODE_UINT;
            logd("STORE_DEREF " UINT_FMT, unum);
            break;

        case MP_BC_STORE_NAME:
            DBG_DECODE_QSTR;
            logd("STORE_NAME %s", qstr_str(qst));
            break;

        case MP_BC_STORE_GLOBAL:
            DBG_DECODE_QSTR;
            logd("STORE_GLOBAL %s", qstr_str(qst));
            break;

        case MP_BC_STORE_ATTR:
            DBG_DECODE_QSTR;
            logd("STORE_ATTR %s", qstr_str(qst));
            if (MICROPY_OPT_CACHE_MAP_LOOKUP_IN_BYTECODE) {
                logd(" (cache=%u)", *ip++);
            }
            break;

        case MP_BC_STORE_SUBSCR:
            logd("STORE_SUBSCR");
            break;

        case MP_BC_DELETE_FAST:
            DBG_DECODE_UINT;
            logd("DELETE_FAST " UINT_FMT, unum);
            break;

        case MP_BC_DELETE_DEREF:
            DBG_DECODE_UINT;
            logd("DELETE_DEREF " UINT_FMT, unum);
            break;

        case MP_BC_DELETE_NAME:
            DBG_DECODE_QSTR;
            logd("DELETE_NAME %s", qstr_str(qst));
            break;

        case MP_BC_DELETE_GLOBAL:
            DBG_DECODE_QSTR;
            logd("DELETE_GLOBAL %s", qstr_str(qst));
            break;

        case MP_BC_DUP_TOP:
            logd("DUP_TOP");
            break;

        case MP_BC_DUP_TOP_TWO:
            logd("DUP_TOP_TWO");
            break;

        case MP_BC_POP_TOP:
            logd("POP_TOP");
            break;

        case MP_BC_ROT_TWO:
            logd("ROT_TWO");
            break;

        case MP_BC_ROT_THREE:
            logd("ROT_THREE");
            break;

        case MP_BC_JUMP:
            DBG_DECODE_SLABEL;
            logd("JUMP " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_POP_JUMP_IF_TRUE:
            DBG_DECODE_SLABEL;
            logd("POP_JUMP_IF_TRUE " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_POP_JUMP_IF_FALSE:
            DBG_DECODE_SLABEL;
            logd("POP_JUMP_IF_FALSE " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_JUMP_IF_TRUE_OR_POP:
            DBG_DECODE_SLABEL;
            logd("JUMP_IF_TRUE_OR_POP " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_JUMP_IF_FALSE_OR_POP:
            DBG_DECODE_SLABEL;
            logd("JUMP_IF_FALSE_OR_POP " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_SETUP_WITH:
            DBG_DECODE_ULABEL; // loop-like labels are always forward
            logd("SETUP_WITH " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_WITH_CLEANUP:
            logd("WITH_CLEANUP");
            break;

        case MP_BC_UNWIND_JUMP:
            DBG_DECODE_SLABEL;
            logd("UNWIND_JUMP " UINT_FMT " %d", (mp_uint_t)(ip + unum - mp_showbc_code_start), *ip);
            ip += 1;
            break;

        case MP_BC_SETUP_EXCEPT:
            DBG_DECODE_ULABEL; // except labels are always forward
            logd("SETUP_EXCEPT " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_SETUP_FINALLY:
            DBG_DECODE_ULABEL; // except labels are always forward
            logd("SETUP_FINALLY " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_END_FINALLY:
            // if TOS is an exception, reraises the exception (3 values on TOS)
            // if TOS is an integer, does something else
            // if TOS is None, just pops it and continues
            // else error
            logd("END_FINALLY");
            break;

        case MP_BC_GET_ITER:
            logd("GET_ITER");
            break;

        case MP_BC_GET_ITER_STACK:
            logd("GET_ITER_STACK");
            break;

        case MP_BC_FOR_ITER:
            DBG_DECODE_ULABEL; // the jump offset if iteration finishes; for labels are always forward
            logd("FOR_ITER " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_POP_EXCEPT_JUMP:
            DBG_DECODE_ULABEL; // these labels are always forward
            logd("POP_EXCEPT_JUMP " UINT_FMT, (mp_uint_t)(ip + unum - mp_showbc_code_start));
            break;

        case MP_BC_BUILD_TUPLE:
            DBG_DECODE_UINT;
            logd("BUILD_TUPLE " UINT_FMT, unum);
            break;

        case MP_BC_BUILD_LIST:
            DBG_DECODE_UINT;
            logd("BUILD_LIST " UINT_FMT, unum);
            break;

        case MP_BC_BUILD_MAP:
            DBG_DECODE_UINT;
            logd("BUILD_MAP " UINT_FMT, unum);
            break;

        case MP_BC_STORE_MAP:
            logd("STORE_MAP");
            break;

        case MP_BC_BUILD_SET:
            DBG_DECODE_UINT;
            logd("BUILD_SET " UINT_FMT, unum);
            break;

#if MICROPY_PY_BUILTINS_SLICE
        case MP_BC_BUILD_SLICE:
            DBG_DECODE_UINT;
            logd("BUILD_SLICE " UINT_FMT, unum);
            break;
#endif

        case MP_BC_STORE_COMP:
            DBG_DECODE_UINT;
            logd("STORE_COMP " UINT_FMT, unum);
            break;

        case MP_BC_UNPACK_SEQUENCE:
            DBG_DECODE_UINT;
            logd("UNPACK_SEQUENCE " UINT_FMT, unum);
            break;

        case MP_BC_UNPACK_EX:
            DBG_DECODE_UINT;
            logd("UNPACK_EX " UINT_FMT, unum);
            break;

        case MP_BC_MAKE_FUNCTION:
            DBG_DECODE_PTR;
            logd("MAKE_FUNCTION %p", (void*)(uintptr_t)unum);
            break;

        case MP_BC_MAKE_FUNCTION_DEFARGS:
            DBG_DECODE_PTR;
            logd("MAKE_FUNCTION_DEFARGS %p", (void*)(uintptr_t)unum);
            break;

        case MP_BC_MAKE_CLOSURE: {
            DBG_DECODE_PTR;
            mp_uint_t n_closed_over = *ip++;
            logd("MAKE_CLOSURE %p " UINT_FMT, (void*)(uintptr_t)unum, n_closed_over);
            break;
        }

        case MP_BC_MAKE_CLOSURE_DEFARGS: {
            DBG_DECODE_PTR;
            mp_uint_t n_closed_over = *ip++;
            logd("MAKE_CLOSURE_DEFARGS %p " UINT_FMT, (void*)(uintptr_t)unum, n_closed_over);
            break;
        }

        case MP_BC_CALL_FUNCTION:
            DBG_DECODE_UINT;
            logd("CALL_FUNCTION n=" UINT_FMT " nkw=" UINT_FMT, unum & 0xff, (unum >> 8) & 0xff);
            break;

        case MP_BC_CALL_FUNCTION_VAR_KW:
            DBG_DECODE_UINT;
            logd("CALL_FUNCTION_VAR_KW n=" UINT_FMT " nkw=" UINT_FMT, unum & 0xff, (unum >> 8) & 0xff);
            break;

        case MP_BC_CALL_METHOD:
            DBG_DECODE_UINT;
            logd("CALL_METHOD n=" UINT_FMT " nkw=" UINT_FMT, unum & 0xff, (unum >> 8) & 0xff);
            break;

        case MP_BC_CALL_METHOD_VAR_KW:
            DBG_DECODE_UINT;
            logd("CALL_METHOD_VAR_KW n=" UINT_FMT " nkw=" UINT_FMT, unum & 0xff, (unum >> 8) & 0xff);
            break;

        case MP_BC_RETURN_VALUE:
            logd("RETURN_VALUE");
            break;

        case MP_BC_RAISE_VARARGS:
            unum = *ip++;
            logd("RAISE_VARARGS " UINT_FMT, unum);
            break;

        case MP_BC_YIELD_VALUE:
            logd("YIELD_VALUE");
            break;

        case MP_BC_YIELD_FROM:
            logd("YIELD_FROM");
            break;

        case MP_BC_IMPORT_NAME:
            DBG_DECODE_QSTR;
            logd("IMPORT_NAME '%s'", qstr_str(qst));
            break;

        case MP_BC_IMPORT_FROM:
            DBG_DECODE_QSTR;
            logd("IMPORT_FROM '%s'", qstr_str(qst));
            break;

        case MP_BC_IMPORT_STAR:
            logd("IMPORT_STAR");
            break;

        default:
            if (ip[-1] < MP_BC_LOAD_CONST_SMALL_INT_MULTI + 64) {
                logd("LOAD_CONST_SMALL_INT " INT_FMT, (mp_int_t)ip[-1] - MP_BC_LOAD_CONST_SMALL_INT_MULTI - 16);
            } else if (ip[-1] < MP_BC_LOAD_FAST_MULTI + 16) {
                logd("LOAD_FAST " UINT_FMT, (mp_uint_t)ip[-1] - MP_BC_LOAD_FAST_MULTI);
            } else if (ip[-1] < MP_BC_STORE_FAST_MULTI + 16) {
                logd("STORE_FAST " UINT_FMT, (mp_uint_t)ip[-1] - MP_BC_STORE_FAST_MULTI);
            } else if (ip[-1] < MP_BC_UNARY_OP_MULTI + MP_UNARY_OP_NUM_BYTECODE) {
                logd("UNARY_OP " UINT_FMT, (mp_uint_t)ip[-1] - MP_BC_UNARY_OP_MULTI);
            } else if (ip[-1] < MP_BC_BINARY_OP_MULTI + MP_BINARY_OP_NUM_BYTECODE) {
                mp_uint_t op = ip[-1] - MP_BC_BINARY_OP_MULTI;
                logd("BINARY_OP " UINT_FMT " %s", op, qstr_str(mp_binary_op_method_name[op]));
            } else {
                logd("code %p, byte code 0x%02x not implemented\n", ip, ip[-1]);
                assert(0);
                return ip;
            }
            break;
    }

    return ip;
}

void mp_bytecode_print2(const byte *ip, size_t len, const mp_uint_t *const_table) {
    mp_showbc_code_start = ip;
    mp_showbc_const_table = const_table;
    while (ip < len + mp_showbc_code_start) {
        logd("%02u ", (uint)(ip - mp_showbc_code_start));
        ip = mp_bytecode_print_str(ip);
        logd("\n");
    }
}
