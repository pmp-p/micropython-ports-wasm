#ifndef VM_DECODERS

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




#define VM_DECODE_QSTR \
    qstr qst = ip[0] | ip[1] << 8; \
    ip += 2;

#define VM_DECODE_PTR \
    DECODE_UINT; \
    void *ptr = (void*)(uintptr_t)CTX.code_state->fun_bc->const_table[unum]

#define VM_DECODE_OBJ \
    DECODE_UINT; \
    mp_obj_t obj = (mp_obj_t)CTX.code_state->fun_bc->const_table[unum]


#endif // VM_DECODERS
