/*
typedef struct _mp_obj_gen_instance_t {
    mp_obj_base_t base;
    mp_obj_dict_t *globals;
    mp_code_state_t code_state;
} mp_obj_gen_instance_t;
*/

//order matters
#define VM_IDLE     0
#define VM_WARMUP   1
#define VM_RUNNING   2
#define VM_RESUMING  3
#define VM_PAUSED    4
#define VM_SYSCALL    5

static int sub_tracking = 0;

struct mp_registers {
    // builtins override dict  ?
    // __main__ ?
    // sys.(std*) ?
    // sys.argv

    // cpu time load stats ?

    //who created us
    unsigned int parent ;

    //
    int sub_id ;

    //who did we create and was running last
    unsigned int childcare ;

    int vmloop_state;
    nlr_buf_t nlr;

    mp_lexer_t *lex;
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
    int sub_alloc ; // child allocated memory size

    mp_vm_return_kind_t vm_return_kind;
    // last
    //mp_obj_gen_instance_t self_gen;
};


//TODO sys max recursion handling.
static struct mp_registers mpi_ctx[SYS_MAX_RECURSION];

static int ctx_current = 1;
static int ctx_next = 2;
#define CTX  mpi_ctx[ctx_current]
#define NEXT mpi_ctx[ctx_next]
#define PARENT mpi_ctx[CTX.parent]

#define JMP_NONE (void*)0
#define TYPE_JUMP 0
#define TYPE_SUB 1

#define JUMPED_IN reached_point[CTX.pointer]
#define JUMP_TYPE type_point[CTX.pointer]
#define ENTRY_POINT entry_point[CTX.pointer]
#define EXIT_POINT exit_point[CTX.pointer]

#define MAX_BRANCHING 128

#define CTX_STATE CTX.vmloop_state
#define NEXT_STATE NEXT.vmloop_state

static int come_from[MAX_BRANCHING];
static int type_point[MAX_BRANCHING];
static int reached_point[MAX_BRANCHING];
static void* entry_point[MAX_BRANCHING];
static void* exit_point[MAX_BRANCHING];
static int point_ptr = 0;  // index 0 is reserved for no branching

static void* crash_point;


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

    mpi_ctx[ctx].sub_id = sub_tracking++;

}



#define CTX_DEBUG 0


void ctx_get_next() {
    //push
    int ctx;
    for (ctx=3; ctx<SYS_MAX_RECURSION; ctx++)
        if (mpi_ctx[ctx].vmloop_state == VM_IDLE){
            // should default to syscall ? VM_PAUSED
            mpi_ctx[ctx].vmloop_state = VM_WARMUP ;
            break;
        }
    if (ctx==SYS_MAX_RECURSION)
        fprintf(stderr,"FATAL: SYS_MAX_RECURSION reached");
    #if CTX_DEBUG
    else
        clog("CTX reservation %d",ctx);
    #endif
    //track
    CTX.childcare = ctx ;
    mp_new_interpreter(&mpi_ctx, ctx, ctx_current, 0);
    ctx_next = ctx;
}

void zigzag(void* jump_entry, void* jump_back, int jump_type){
    come_from[++point_ptr] = CTX.pointer; // when set to 0 it's origin mark.
    CTX.pointer = point_ptr;
    JUMPED_IN = 0;
    ENTRY_POINT = jump_entry;
    EXIT_POINT = jump_back;
    JUMP_TYPE = jump_type;
}

void ctx_switch(){
    NEXT_STATE  = VM_RUNNING;
    ctx_current = ctx_next;
    #if CTX_DEBUG
    clog("CTX %d locked for %d",ctx_current,CTX.parent);
    #endif
}

void* ctx_sub(void* jump_entry, void* jump_back, const char* jto, const char* jback, const char* context) {
    // set the new context so zigzag @ are set
    ctx_switch();

    zigzag(jump_entry, jump_back, TYPE_SUB);
    clog("    Begin[%d:%d] %s(...) %s -> %s  %d->%d", ctx_current, CTX.sub_id, context, jto, jback,  CTX.parent, ctx_current);
    JUMPED_IN = 1 ;
    return jump_entry;
}


#define GOSUB(arg0, arg1, arg2) goto *ctx_sub(&&arg0, &&arg1, TOSTRING(arg0), TOSTRING(arg1), arg2 )

void *crash(const char *panic){
    clog("%s", panic);
    return crash_point;

}

void ctx_release(){
    #if CTX_DEBUG
    clog("CTX %d released from %d",ctx_next,ctx_current);
    #endif
    NEXT_STATE = VM_IDLE;
}

void ctx_free(){
    #if 1 // CTX_DEBUG
    clog("CTX %d freed for %d", ctx_current, CTX.parent);
    #endif
    PARENT.childcare = 0;
    CTX_STATE = VM_IDLE;
}

void* ctx_come_from() {
    // just going back from branching
    int ptr_back = come_from[CTX.pointer];

    void* return_point;

    if (JUMP_TYPE!=TYPE_JUMP)
        return crash("BRANCH EXIT in SUB CONTEXT");

    return_point = EXIT_POINT;

    if (point_ptr!=CTX.pointer) {
        return crash("jumping back from upper branch not allowed");
    } else
        point_ptr--;

    // go up one level of branching ( or if 0 reach root )
    CTX.pointer = ptr_back;
    return return_point;
}



void* ctx_return(){
    void* return_point;
    int ptr_back = come_from[CTX.pointer];

    PARENT.sub_value = CTX.return_value;
    PARENT.sub_alloc = CTX.alloc;

    // possibly return in upper branch ?
    if (point_ptr!=CTX.pointer) {
        clog("ERROR not on leaf branch");
        emscripten_cancel_main_loop();
    } else {
        point_ptr -= 1;
    }
    return_point = exit_point[CTX.pointer];


    // not a zigzag branching free registers
    if (!ptr_back)
        ctx_free();
    else {
        return crash("ERROR: RETURN in BRANCH");
    }

    // not a zigzag go upper context
    if ( JUMP_TYPE == TYPE_SUB)
        ctx_current = CTX.parent;
    else
        return crash("ERROR: RETURN in BRANCH");

    return return_point;
}


void* ctx_branch(void* jump_entry, void *jump_back, const char *jto, const char *jback, const char* context) {
    zigzag(jump_entry, jump_back, TYPE_JUMP);
    clog("    ZZZ> %s(...) %s -> %s  @%d",context, jto, jback, ctx_current);
    JUMPED_IN = 1 ;
    return jump_entry;
}


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define BRANCH(arg0, arg1, arg2) goto *ctx_branch(&&arg0, &&arg1, TOSTRING(arg0), TOSTRING(arg1), arg2)

#define FATAL(msg) goto *crash(msg)

#define SUBVAL CTX.sub_value
#define RETVAL CTX.return_value

#define RETURN goto *ctx_return()
#define COME_FROM goto *ctx_come_from()


// mp_obj_t mp_import_name(qstr qst, mp_obj_t fromlist, mp_obj_t level)

static qstr sub_import_name_qst ;
static mp_obj_t sub_import_name_fromlist ;
static mp_obj_t sub_import_name_level ;

