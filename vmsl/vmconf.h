//TODO sys max recursion handling.
#define SYS_MAX_RECURSION 32
#define MAX_BRANCHING 128

//order matters
#define VM_IDLE     0
#define VM_WARMUP   1
#define VM_RUNNING   2
#define VM_RESUMING  3
#define VM_PAUSED    4
#define VM_SYSCALL    5
#define VM_H_CF_OR_SC  6

static int VMOP = -1;

#define VMOP_NONE       0
#define VMOP_INIT       1
#define VMOP_WARMUP     2
#define VMOP_CALL       3
#define VMOP_CRASH      4
#define VMOP_PAUSE      5
#define VMOP_SYSCALL    6



static int sub_tracking = 0;

static int ctx_current = 1;
static int ctx_next = 2;

//need interrupt state marker ( @syscall / @awaited  )
static int ctx_sti = 0 ;

