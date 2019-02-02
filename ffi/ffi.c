
#include "ffi.h"
#include "ffi_common.h"
#include <stdint.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#else
    #define EMSCRIPTEN_KEEPALIVE
#endif



/*

ffi_prep_closure_loc BROKEN

*/


ffi_status
ffi_prep_closure_loc (ffi_closure* closure,
                      ffi_cif* cif,
                      void (*fun)(ffi_cif*, void*, void**, void*),
                      void *user_data,
                      void *codeloc)
{
  closure->cif = cif;
  closure->fun = fun;
  closure->user_data = user_data;
  return FFI_OK;
}



void *
ffi_closure_alloc (size_t size, void **code)
{
    return 0;
}





















ffi_status FFI_HIDDEN
ffi_prep_cif_machdep(ffi_cif *cif)
{
    return FFI_OK;
}

void
ffi_call (ffi_cif *cif, void (*fn)(void), void *rvalue, void **avalue)
{
    EM_ASM_ ({
        var cif = $0;
        var fn = $1;
        var rvalue = $2;
        var avalue = $3;

        var cif_abi = HEAPU32[cif >> 2];
        var cif_nargs = HEAPU32[(cif + 4) >> 2];
        var cif_arg_types = HEAPU32[(cif + 8) >> 2];
        var cif_rtype = HEAPU32[(cif + 12) >> 2];

        // emscripten/wasm function pointers are indirected through a table,
        // which is further subdivided by function signature -- each pointer
        // must be added to a constant to find its real sig. This doesn't seem
        // to be a strict wasm requirement, but is a leftover from asm.js
        // which used separate arrays for each signature.
        //
        // This is neatly encapsulated into dynCall_* wrapper functions for us,
        // which take a function pointer as first parameter and pass the rest on.
        //
        // Generate an emscripten call signature and look up the correct wrapper
        // via name here...
        var args = [fn];
        var sig = "";
        var sigFloat = Module['usingWasm'] ? "f" : "d";

        var rtype = HEAPU16[(cif_rtype + 6 /* rtype->type*/ ) >> 1];
        //console.error('rtype is', rtype);
        if (rtype === /* FFI_TYPE_VOID */ 0) {
            sig = 'v';
        } else if (rtype === /* FFI_TYPE_INT */ 1 ||
                   rtype === /* FFI_TYPE_UINT8 */ 5 ||
                   rtype === /* FFI_TYPE_SINT8 */ 6 ||
                   rtype === /* FFI_TYPE_UINT16 */ 7 ||
                   rtype === /* FFI_TYPE_SINT16 */ 8 ||
                   rtype === /* FFI_TYPE_UINT32 */ 9 ||
                   rtype === /* FFI_TYPE_SINT32 */ 10 ||
                   rtype === /* FFI_TYPE_POINTER */ 14) {
            sig = 'i';
        } else if (rtype === /* FFI_TYPE_FLOAT */ 2) {
            sig = sigFloat;
        } else if (rtype === /* FFI_TYPE_DOUBLE */ 3 ||
                   rtype === /* FFI_TYPE_LONGDOUBLE */ 4) {
            sig = 'd';
        } else if (rtype === /* FFI_TYPE_UINT64 */ 11 ||
                   rtype === /* FFI_TYPE_SINT64 */ 12) {
            // Warning: returns a truncated 32-bit integer directly.
            // High bits are in $tempRet0
            sig = 'j';
        } else if (rtype === /* FFI_TYPE_STRUCT */ 13) {
            throw new Error('struct ret marshalling nyi');
        } else if (rtype === /* FFI_TYPE_COMPLEX */ 15) {
            throw new Error('complex ret marshalling nyi');
        } else {
            throw new Error('Unexpected rtype ' + rtype);
        }

        for (var i = 0; i < cif_nargs; i++) {
            var ptr = HEAPU32[(avalue >> 2) + i];

            var arg_type = HEAPU32[(cif_arg_types >> 2) + i];
            var typ = HEAPU16[(arg_type + 6) >> 1];

            if (typ === /* FFI_TYPE_INT*/ 1 || typ === /* FFI_TYPE_SINT32 */ 10) {
                args.push(HEAP32[ptr >> 2]);
                sig += 'i';
            } else if (typ === /* FFI_TYPE_FLOAT */ 2) {
                args.push(HEAPF32[ptr >> 2]);
                sig += sigFloat;
            } else if (typ === /* FFI_TYPE_DOUBLE */ 3 || typ === /* FFI_TYPE_LONGDOUBLE */ 4) {
                args.push(HEAPF64[ptr >> 3]);
                sig += 'd';
            } else if (typ === /* FFI_TYPE_UINT8*/ 5) {
                args.push(HEAPU8[ptr]);
                sig += 'i';
            } else if (typ === /* FFI_TYPE_SINT8 */ 6) {
                args.push(HEAP8[ptr]);
                sig += 'i';
            } else if (typ === /* FFI_TYPE_UINT16 */ 7) {
                args.push(HEAPU16[ptr >> 1]);
                sig += 'i';
            } else if (typ === /* FFI_TYPE_SINT16 */ 8) {
                args.push(HEAP16[ptr >> 1]);
                sig += 'i';
            } else if (typ === /* FFI_TYPE_UINT32 */ 9 || typ === /* FFI_TYPE_POINTER */ 14) {
                args.push(HEAPU32[ptr >> 2]);
                sig += 'i';
            } else if (typ === /* FFI_TYPE_UINT64 */ 11 || typ === /* FFI_TYPE_SINT64 */ 12) {
                // LEGALIZE_JS_FFI mode splits i64 (j) into two i32 args
                // for compatibility with JavaScript's f64-based numbers.
                args.push(HEAPU32[ptr >> 2]);
                args.push(HEAPU32[(ptr + 4) >> 2]);
                sig += 'j';
            } else if (typ === /* FFI_TYPE_STRUCT */ 13) {
                throw new Error('struct marshalling nyi');
            } else if (typ === /* FFI_TYPE_COMPLEX */ 15) {
                throw new Error('complex marshalling nyi');
            } else {
                throw new Error('Unexpected type ' + typ);
            }
        }

        //console.error('fn is',fn);
        //console.error('sig is',sig);
        var func = Module['dynCall_' + sig];
        //console.error('func is', func);
        //console.error('args is', args);
        if (func) {
          var result = func.apply(null, args);
        } else {
          console.error('fn is', fn);
          console.error('sig is', sig);
          console.error('args is', args);
          for (var x in Module) {
            console.error('-- ' + x);
          }
          throw new Error('invalid function pointer in ffi_call');
        }
        //console.error('result is',result);

        if (rtype === 0) {
            // void
        } else if (rtype === 1 || rtype === 9 || rtype === 10 || rtype === 14) {
            HEAP32[rvalue >> 2] = result;
        } else if (rtype === 2) {
            HEAPF32[rvalue >> 2] = result;
        } else if (rtype === 3 || rtype === 4) {
            HEAPF64[rvalue >> 3] = result;
        } else if (rtype === 5 || rtype === 6) {
            HEAP8[rvalue] = result;
        } else if (rtype === 7 || rtype === 8) {
            HEAP16[rvalue >> 1] = result;
        } else if (rtype === 11 || rtype === 12) {
            // Warning: returns a truncated 32-bit integer directly.
            // High bits are in $tempRet0
            HEAP32[rvalue >> 2] = result;
            HEAP32[(rvalue + 4) >> 2] = Module.getTempRet0();
        } else if (rtype === 13) {
            throw new Error('struct ret marshalling nyi');
        } else if (rtype === 15) {
            throw new Error('complex ret marshalling nyi');
        } else {
            throw new Error('Unexpected rtype ' + rtype);
        }
    }, cif, fn, rvalue, avalue);
}

