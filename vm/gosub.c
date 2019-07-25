typedef struct _mp_obj_gen_instance_t {
    mp_obj_base_t base;
    mp_obj_dict_t *globals;
    mp_code_state_t code_state;
} mp_obj_gen_instance_t;


struct mp_registers {
    // builtins override dict  ?
    // __main__ ?
    // sys.(std*) ?
    // sys.argv

    // cpu time load stats ?

    //who created us
    unsigned int parent ;

    //who did we create and was running last
    unsigned int childcare ;

    int vmloop_state;
    unsigned long coropass;

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


//  mp_call_function_n_kw state  and iterators state
    mp_obj_t self_in;
    mp_obj_fun_bc_t *self;

    //mp_obj_type_t *type;

    size_t n_args;
    size_t n_kw;
    const mp_obj_t *args;

    mp_obj_gen_instance_t* generator;
    mp_obj_t send_value, throw_value, return_value;

    mp_obj_t sub_value;  // child ctx computed value.

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


#define ENTRY_POINT entry_point[CTX.pointer]
#define EXIT_POINT exit_point[CTX.pointer]



static int come_from[128];
static void* entry_point[128];
static void* exit_point[128];
static int point_ptr = 0;  // index 0 is reserved for no branching



void mp_new_interpreter(void * mpi, int ctx, int parent, int childcare) {
    mpi_ctx[ctx].vmloop_state = VM_IDLE;
    mpi_ctx[ctx].parent = parent;
    mpi_ctx[ctx].childcare = childcare;
    mpi_ctx[ctx].pointer = 0;
    mpi_ctx[ctx].code_state = NULL ;

    mpi_ctx[ctx].n_args = 0;
    mpi_ctx[ctx].n_kw = 0;
    mpi_ctx[ctx].args = &mpi_ctx[ctx].argv[0];
}

void ctx_get_next() {
    //push
    int ctx;
    for (ctx=3; ctx<SYS_MAX_RECURSION; ctx++)
        if (mpi_ctx[ctx].vmloop_state == VM_IDLE){
            // should default to syscall ? VM_PAUSED
            mpi_ctx[ctx].vmloop_state = VM_RUNNING ;
            break;
        }
    //track
    CTX.childcare = ctx ;
    mp_new_interpreter(&mpi_ctx, ctx, ctx_current, 0);
    ctx_next = ctx;
}

void zigzag(void* jump_entry, void* jump_back){
    come_from[++point_ptr] = CTX.pointer; // when set to 0 it's origin mark.
    CTX.pointer = point_ptr;
    ENTRY_POINT = jump_entry;
    EXIT_POINT = jump_back;
}

void* ctx_sub(void* jump_entry, void* jump_back, const char* jto, const char* jback, const char* context) {
    ctx_current = ctx_next ; // set the new context so zigzag @ are set
    zigzag(jump_entry, jump_back);
    clog("    SUB> %s(...) %s -> %s  %d->%d", context, jto, jback, come_from[CTX.pointer], CTX.pointer);
    return jump_entry;
}


#define GOSUB(arg0, arg1, arg2) goto *ctx_sub(&&arg0, &&arg1, TOSTRING(arg0), TOSTRING(arg1), arg2 )

// TODO: checks
void ctx_release(){
    NEXT.vmloop_state = VM_IDLE;
}

void ctx_free(){
    CTX.vmloop_state = VM_IDLE;
}


void* ctx_return(){
    void* return_point;
    // just going back from branching
    int ptr_back = come_from[CTX.pointer];

    if (!ptr_back) {
        PARENT.childcare = 0;
        CTX.vmloop_state = VM_IDLE;

    }

    PARENT.sub_value = CTX.return_value;

    if (point_ptr!=CTX.pointer) {
        clog("ERROR not on leaf branch");
        emscripten_cancel_main_loop();
    } else {
        point_ptr -= 1;
    }
    return_point = exit_point[CTX.pointer];
    CTX.pointer = ptr_back; // up one level of branching

    // if not a zigzag branching go upper context and free registers
    if (!ptr_back) {
        ctx_free();
        ctx_current = CTX.parent;
    }
    return return_point;
}


void* ctx_branch(void* jump_entry, void *jump_back, const char *jto, const char *jback, const char* context) {
    zigzag(jump_entry, jump_back);
    clog("    ZZZ> %s(...) %s -> %s  %d->%d",context, jto, jback, come_from[CTX.pointer], CTX.pointer);
    return jump_entry;
}


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define BRANCH(arg0, arg1, arg2) goto *ctx_branch(&&arg0, &&arg1, TOSTRING(arg0), TOSTRING(arg1), arg2)


#define RETVAL CTX.return_value

#define RETURN goto *ctx_return()


// mp_obj_t mp_import_name(qstr qst, mp_obj_t fromlist, mp_obj_t level)

static qstr sub_import_name_qst ;
static mp_obj_t sub_import_name_fromlist ;
static mp_obj_t sub_import_name_level ;

