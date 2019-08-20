#include "lib/bipbuffer/bipbuffer.h"
#include "lib/bipbuffer/bipbuffer.c"


#ifndef VM_REG_H

    #define clog(...) { fprintf(stderr, __VA_ARGS__ );fprintf(stderr, "\n"); }
    struct mp_registers {
        int sub_id ;
        //who created us
        int parent ;
        //who did we create and was running last
        int childcare ;

        // execution path
        int pointer;
        int switch_break_for;
        int vmloop_state;
    };

    static struct mp_registers mpi_ctx[SYS_MAX_RECURSION];

    void mp_new_interpreter(void * mpi, int ctx, int parent, int childcare) {

        mpi_ctx[ctx].parent = parent;
        mpi_ctx[ctx].childcare = childcare;
    //
        mpi_ctx[ctx].pointer = 0;
        mpi_ctx[ctx].sub_id = sub_tracking++;
        mpi_ctx[ctx].vmloop_state = VM_IDLE;
    }
#endif


#define JMP_NONE (void*)0
#define TYPE_JUMP 0
#define TYPE_SUB 1

#define JUMPED_IN reached_point[CTX.pointer]
#define JUMP_TYPE type_point[CTX.pointer]
#define ENTRY_POINT entry_point[CTX.pointer]
#define EXIT_POINT exit_point[CTX.pointer]

static int point_ptr = 0;  // index 0 is reserved for no branching

static int come_from[MAX_BRANCHING];
static int type_point[MAX_BRANCHING];
static int reached_point[MAX_BRANCHING];
static void* entry_point[MAX_BRANCHING];
static void* exit_point[MAX_BRANCHING];


static void* crash_point = JMP_NONE;





static struct mp_registers mpi_ctx[SYS_MAX_RECURSION];

#define CTX  mpi_ctx[ctx_current]
#define NEXT mpi_ctx[ctx_next]
#define PARENT mpi_ctx[CTX.parent]

#define CTX_STATE CTX.vmloop_state
#define NEXT_STATE NEXT.vmloop_state

void *crash(const char *panic){
    clog("%s", panic);
    VMOP = VMOP_CRASH;
    return crash_point;

}

#define FATAL(msg) goto *crash(msg)

#if VM_FULL
    #define SUBVAL CTX.sub_value
    #define RETVAL CTX.return_value
#endif

#define RETURN goto *ctx_return()
#define COME_FROM goto *ctx_come_from()



void ctx_free(){
    clog("CTX %d free for %d", ctx_current, CTX.parent);
    CTX_STATE = VM_IDLE;
}

static int come_from[MAX_BRANCHING];

#define deferred 1

void zigzag(void* jump_entry, void* jump_back, int jump_type){
    come_from[++point_ptr] = CTX.pointer; // when set to 0 its origin mark.
    CTX.pointer = point_ptr;
    JUMPED_IN = 0;
    ENTRY_POINT = jump_entry;
    EXIT_POINT = jump_back;
    JUMP_TYPE = jump_type;
}




#define CTX_COPY 1
#define CTX_NEW 0

// prepare a child context for a possibly recursive call,
// NEXT.*  will then gives access to that ctx (inited from CTX if CTX_COPY ) until GOSUB()
// after GOSUB CTX is restored automaticallly to parent and return value is in CTX.sub_value

void ctx_get_next(int copy) {
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

    if (copy) {
        NEXT.self_in = CTX.self_in;
        NEXT.code_state = CTX.code_state ;
        NEXT.n_args = CTX.n_args;
        NEXT.n_kw = CTX.n_kw;
        NEXT.args = CTX.args;
    }
}


void ctx_switch(){
    NEXT_STATE  = VM_RUNNING;
    ctx_current = ctx_next;
    #if CTX_DEBUG
    clog("CTX %d locked for %d",ctx_current,CTX.parent);
    #endif
}

void* ctx_branch(void* jump_entry,int vmop, void *jump_back, const char *jto, const char *jback, const char* context, int defer) {
    VMOP = vmop;
    zigzag(jump_entry, jump_back, TYPE_JUMP);
    clog("    ZZ > %s(...) %s -> %s  @%d",context, jto, jback, ctx_current);
    if (!defer)
        JUMPED_IN = 1 ;
    return jump_entry;
}

#define BRANCH(arg0, vmop, arg1, arg2) goto *ctx_branch(&&arg0, vmop, &&arg1, TOSTRING(arg0), TOSTRING(arg1), arg2, !deferred)

#define SYSCALL(arg0, vmop, arg1, arg2) \
{\
 ctx_branch(&&arg0, vmop, &&arg1, TOSTRING(arg0), TOSTRING(arg1), arg2, deferred);\
 goto VM_syscall;\
}


void* ctx_call(void* jump_entry, void *jump_back, const char *jt_origin,const char *context, int defer) {
    VMOP = VMOP_CALL;
    zigzag(jump_entry, jump_back, TYPE_JUMP);
    clog("    CC > %s->%s(...) @%d", context, jt_origin, ctx_current);
    if (!defer)
        JUMPED_IN = 1 ;
    return jump_entry;
}

#define CALL(arg0, caller) \
{\
 ctx_call(&&arg0, &&CONCAT(call_,__LINE__), TOSTRING(arg0), caller, deferred);\
 goto VM_syscall;\
 CONCAT(call_,__LINE__):;\
}

#define JUMP(arg0, caller) \
{\
 goto *ctx_call(&&arg0, &&CONCAT(call_,__LINE__), TOSTRING(arg0), caller, !deferred);\
 CONCAT(call_,__LINE__):;\
}

void* ctx_sub(void* jump_entry, void* jump_back, const char* jto, const char* jback, const char* context) {
    // set the new context so zigzag @ are set
    ctx_switch();

    zigzag(jump_entry, jump_back, TYPE_SUB);
    clog("    Begin[%d:%d] %s(...) %s -> %s  %d->%d", ctx_current, CTX.sub_id, context, jto, jback,  CTX.parent, ctx_current);
    JUMPED_IN = 1 ;
    return jump_entry;
}
//#define GOSUB(arg0, arg1, arg2) goto *ctx_sub(&&arg0, &&arg1, TOSTRING(arg0), TOSTRING(arg1), arg2 )

#define GOSUB(arg0, caller) \
{\
 goto *ctx_sub(&&arg0, &&CONCAT(call_,__LINE__), TOSTRING(arg0), TOSTRING(CONCAT(call_,__LINE__)), caller ) ;\
 CONCAT(call_,__LINE__):;\
}


void* ctx_come_from() {
    // just going back from branching
    int ptr_back = come_from[CTX.pointer];

    void* return_point;

    if (JUMP_TYPE!=TYPE_JUMP)
        return crash("BRANCH EXIT in SUB CONTEXT");

    return_point = EXIT_POINT;

    if (point_ptr!=CTX.pointer) {
        return crash("ERROR: jumping back from upper branch not allowed, maybe ctx_get_next missing for GOSUB ?");
    } else
        point_ptr--;
    clog("<<< ZZ[%d]",ctx_current);
    // go up one level of branching ( or if 0 reach root )
    CTX.pointer = ptr_back;
    return return_point;
}



void* ctx_return(){
    void* return_point;
    int ptr_back = come_from[CTX.pointer];

#if VM_FULL
    PARENT.sub_vm_return_kind = CTX.vm_return_kind;
    PARENT.sub_value = CTX.return_value;
    PARENT.sub_alloc = CTX.alloc;
    PARENT.sub_args  = CTX.args ;
#endif

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

    // not a zigzag go upper context
    if ( JUMP_TYPE == TYPE_SUB)
        ctx_current = CTX.parent;
    else
        return crash("239:ERROR: RETURN in non SUB BRANCH");

    return return_point;
}




#define STRINGIFY(a) #a
#define TOSTRING(x) STRINGIFY(x)

#define STRINGIFY2(a, b) a ## b
#define CONCAT(x, y) STRINGIFY2(x, y)

struct mp_stack {
    int gen_i;
    int gen_n;
    int has_default;
    int default_value;
    int __line__ ;
    const char *name;
    const char *value;  // some in memory rpc struct
};


// Duff's device thanks a lot to https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html


#define Begin switch(generator_context->__line__) { case 0:

#define yield(x)\
do {\
    generator_context->__line__ = __LINE__;\
    return x; \
case __LINE__:; } while (0)

#define End };


#define STRINGIFY(a) #a
#define TOSTRING(x) STRINGIFY(x)

// static int enumerator=0;
#define vars(generator_name) static struct mp_stack generator_context = { .gen_i=0, .gen_n = 1, .has_default = 0, .default_value = 666, .__line__ = 0, .name=TOSTRING(generator_name) }

#define async_def(generator_name, gen_type, ...) \
	gen_type generator_name(struct mp_stack *generator_context) { \
        Begin; \
		__VA_ARGS__ \
        End;\
        generator_context->gen_n = 0;\
        return generator_context->default_value;\
	}\

#define async_iter(generator_name) \
    vars(generator_name);\
    if (generator_context.gen_n) generator_context.gen_i=generator_name(&generator_context); \
    if (generator_context.gen_n)

#define async_next(generator_name, defval) \
    vars(generator_name); if (!generator_context.has_default) { generator_context.has_default = 1; generator_context.default_value = defval; }\
    int had_next = generator_context.gen_n; \
    if (had_next) generator_context.gen_i = generator_name(&generator_context); \
    if (had_next)


// -------------------------- attempt to be pythonic --------------------
#define self (*generator_context)
#define iterator generator_context->gen_i
#define next generator_context->gen_n


async_def(gen1, int, {
    while (next) {
        printf("gen1 %s ",self.name);
        if (iterator++ == 10) break;
        yield(iterator);
    }
});


async_def(gen2, int, {
    for (iterator = 0; iterator < 5; iterator++) {
        printf("gen2 %s ",self.name);
        yield(iterator);
    }
});

#undef self
#undef iterator
#undef next

#define iterator generator_context.gen_i

// ------------------------------------------------------------------------


// #define FUN_NAME "FUN_NAME"



