#include "py/obj.h"
#include "py/runtime.h"

typedef struct _mp_obj_closure_t {
    mp_obj_base_t base;
    mp_obj_t fun;
    size_t n_closed;
    mp_obj_t closed[];
} mp_obj_closure_t;


typedef struct _mp_obj_gen_instance_t {
    mp_obj_base_t base;
    mp_obj_dict_t *globals;
    mp_code_state_t code_state;
} mp_obj_gen_instance_t;

#include "vmsl/vmconf.h"

struct mp_registers {
    // builtins override dict  ?
    // __main__ ?
    // sys.(std*) ?
    // sys.argv

    // cpu time load stats ?

    //who created us
    int parent ;

    //
    int sub_id ;

    //who did we create and was running last
    int childcare ;

    int vmloop_state;
    nlr_buf_t nlr;

    mp_lexer_t *lex;
    mp_reader_t reader;

    qstr source_name;
    mp_parse_tree_t parse_tree;

    mp_obj_t * /*const*/ fastn;
    mp_exc_stack_t * /*const*/ exc_stack;

    size_t n_state;
    size_t state_size;

    volatile mp_obj_t inject_exc;
    mp_code_state_t *code_state;
    mp_exc_stack_t *volatile exc_sp;

// execution path
    int pointer;
    int switch_break_for;

// parent return state
    size_t ulab;
    size_t slab;
    mp_obj_t *sp;
    const byte *ip;
    mp_obj_t obj_shared;
    void *ptr;

// mp_import state
    qstr qst;
    mp_obj_t argv[5];


//  mp_call_function_n_kw, closure_call and iterators state
    mp_obj_t self_in;
    mp_obj_fun_builtin_var_t *self_fb;
    mp_obj_fun_bc_t *self_fun;
    mp_obj_closure_t *self_clo;

    int alloc;

    //mp_obj_type_t *type;

    size_t n_args;
    size_t n_kw;
    //const
    mp_obj_t *args;

    mp_obj_gen_instance_t* generator;
    mp_obj_t send_value, throw_value, return_value;

    mp_obj_t sub_value;  // child ctx computed value.
    mp_obj_t *sub_args ; // child allocated memory ptr
    int sub_alloc ; // child allocated memory size
    mp_vm_return_kind_t sub_vm_return_kind; // child result on mp_execute_bytecode() calls ( can be recursive )

    mp_vm_return_kind_t vm_return_kind;
    // last
    //mp_obj_gen_instance_t self_gen;
};

static struct mp_registers mpi_ctx[SYS_MAX_RECURSION];

void mp_new_interpreter(void * mpi, int ctx, int parent, int childcare) {
    mpi_ctx[ctx].vmloop_state = VM_IDLE;
    mpi_ctx[ctx].parent = parent;
    mpi_ctx[ctx].childcare = childcare;
    mpi_ctx[ctx].pointer = 0;
    mpi_ctx[ctx].code_state = NULL ;

    mpi_ctx[ctx].n_args = 0;
    mpi_ctx[ctx].n_kw = 0;
    mpi_ctx[ctx].args = &mpi_ctx[ctx].argv[0];
    mpi_ctx[ctx].alloc = 0;
    mpi_ctx[ctx].switch_break_for = 0;
    mpi_ctx[ctx].sub_id = sub_tracking++;

}

#define VM_FULL 1

#define VM_REG_H 1
