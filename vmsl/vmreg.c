#include "lib/bipbuffer/bipbuffer.h"
#include "lib/bipbuffer/bipbuffer.c"


#ifndef VM_REG_H
    #include "vmsl/vmconf.h"

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
    #if 1 // CTX_DEBUG
    clog("CTX %d freed for %d", ctx_current, CTX.parent);
    #endif
    CTX_STATE = VM_IDLE;
}

static int come_from[MAX_BRANCHING];

#define deferred 1

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


void* ctx_sub(void* jump_entry, void* jump_back, const char* jto, const char* jback, const char* context) {
    // set the new context so zigzag @ are set
    ctx_switch();

    zigzag(jump_entry, jump_back, TYPE_SUB);
    clog("    Begin[%d:%d] %s(...) %s -> %s  %d->%d", ctx_current, CTX.sub_id, context, jto, jback,  CTX.parent, ctx_current);
    JUMPED_IN = 1 ;
    return jump_entry;
}
#define GOSUB(arg0, arg1, arg2) goto *ctx_sub(&&arg0, &&arg1, TOSTRING(arg0), TOSTRING(arg1), arg2 )


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
    clog("<<< ZZ[%d]",ctx_current);
    // go up one level of branching ( or if 0 reach root )
    CTX.pointer = ptr_back;
    return return_point;
}



void* ctx_return(){
    void* return_point;
    int ptr_back = come_from[CTX.pointer];

#if VM_FULL
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




#define STRINGIFY(a) #a
#define TOSTRING(x) STRINGIFY(x)

#define STRINGIFY2(a, b) a ## b
#define CONCAT(x, y) STRINGIFY2(x, y)






static int cond1 = 1;
#define FUN_NAME "some_name"



// check_timer(): check if that timer occurred
EM_JS(bool, check_timer, (), {
  return Module.timer;
});



