BASENAME=micropython
PROG=$(BASENAME).html
LIBMICROPYTHON = lib$(BASENAME).a
FROZEN_MPY_DIR = modules
FROZEN_DIR = flash
# clang has slightly different options to GCC
CLANG = 1
CROSS = 0


include ../../py/mkenv.mk

EM_CACHE ?= ~/.emscripten_cache

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

PORTS=$(EM_CACHE)/asmjs/ports-builds

# actually for lvgl
CPPFLAGS=-I$(PORTS)/sdl2/include

ifdef WASM_FILE_API
	CPPFLAGS += -DWASM_FILE_API=1 
endif

ifdef EMSCRIPTEN
	CC = emcc
	CPP = clang -E -D__CPP__ -D__EMSCRIPTEN__
	CPP += --sysroot $(EMSCRIPTEN)/system
	CPP += -isystem $(EMSCRIPTEN)/system/include/libc
	CPP += -isystem $(EMSCRIPTEN)/system/include/libcxx
	CPP += $(CPPFLAGS)
	# Act like 'emcc'
	CPP += -U__i386 -U__i386 -Ui386 -U__SSE -U__SSE_MATH -U__SSE2 -U__SSE2_MATH -U__MMX__ -U__SSE__ -U__SSE_MATH__ -U__SSE2__ -U__SSE2_MATH__
else
	ifdef CLANG
		CC=clang
		CPP=clang -E -D__CPP__
	else
		CC = gcc
		CPP = gcc -E -D__CPP__
	endif
endif



ifdef ASYNC
	CFLAGS += -D__EMTERPRETER__=1
endif

ifdef LVGL
	LVOPTS = -DMICROPY_PY_LVGL=1
	CFLAGS += $(LVOPTS)
	CC += $(LVOPTS) -s USE_SDL=2
	CFLAGS_USERMOD += $(LVOPTS) -s USE_SDL=2 -Wno-unused-function -Wno-for-loop-analysis
	CPP += -DMICROPY_PY_LVGL=1
endif

# include py core make definitions
include ../../py/py.mk

CLANG = 1
SIZE = echo
LD = $(CC)

INC += -I.
INC += -I../..
INC += -I$(BUILD)



CFLAGS = $(INC) -Wall -Werror -ansi -std=gnu99

#Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0
CC += -g4
LD += -g4
else
CFLAGS += -O3 -DNDEBUG
endif

# //for qstr.c
CFLAGS +=-DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool

# //for build/genhdr/qstr.i.last
QSTR_GEN_EXTRA_CFLAGS=-DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool

MPY_CROSS_FLAGS += -mcache-lookup-bc

LD = $(CC)
LDFLAGS = -Wl,-map,$@.map -Wl,-dead_strip -Wl,-no_pie
LDFLAGS += -s EXPORTED_FUNCTIONS="['_main', '_repl','_writecode', '_Py_InitializeEx', '_PyRun_SimpleString', '_PyRun_VerySimpleFile', '_repl_init']"

SRC_C = \
	upython.c \
	wasm_mphal.c \
	file.c \
	modos.c \
	modtime.c \
	moduos_vfs.c \


#optionnal experimental FFI
SRC_C+= \
	modffi.c \
	ffi/types.c \
	ffi/prep_cif.c \

SRC_LIB= $(addprefix lib/, \
	utils/stdout_helpers.c \
	utils/pyexec.c \
	utils/interrupt_char.c \
        mp-readline/readline.c \
	)

SRC_MOD+=modembed.c

ifdef LVGL
	LIB_SRC_C = $(addprefix lib/,\
    	lv_bindings/driver/SDL/SDL_monitor.c \
        lv_bindings/driver/SDL/SDL_mouse.c \
        lv_bindings/driver/SDL/modSDL.c \
        $(LIB_SRC_C_EXTRA) \
        timeutils/timeutils.c \
	)
	CFLAGS+=  -Wno-unused-function -Wno-for-loop-analysis
endif


# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(LIB_SRC_C)

# Append any auto-generated sources that are needed by sources listed in
# SRC_QSTR
SRC_QSTR_AUTO_DEPS += SRC_QSTR


OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_LIB:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_MOD:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(LIB_SRC_C:.c=.o))

all: $(PROG)

include ../../py/mkrules.mk

clean:
	$(RM) -rf $(BUILD) $(CLEAN_EXTRA) $(LIBMICROPYTHON)
	$(shell rm $(BASENAME)/$(BASENAME).* || echo echo test data cleaned up)
		
LIBMICROPYTHON = lib$(BASENAME)$(TARGET).a

#one day, maybe go via a *embeddable* static lib first  ?


#see https://github.com/emscripten-core/emscripten/issues/7811
#COPT+= -s EXPORT_ALL=1

# https://github.com/emscripten-core/emscripten/wiki/Linking

LOPT=-s EXPORT_ALL=1 -s WASM=1 -s SIDE_MODULE=1 -s TOTAL_MEMORY=512MB

libs: $(OBJ)
	$(ECHO) "Linking static $(LIBMICROPYTHON)"
	$(Q)$(AR) rcs $(LIBMICROPYTHON) $(OBJ)
	$(ECHO) "Linking shared lib$(BASENAME)$(TARGET).wasm"
	$(Q)$(LD) $(LDFLAGS) $(LOPT) -o lib$(BASENAME)$(TARGET).wasm $(OBJ) -ldl -lc


# EMOPTS+= -Oz -g0 -s FORCE_FILESYSTEM=1  --memory-init-file 1

COPT += -s ENVIRONMENT=web -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1

COPT += -s ASSERTIONS=2 -s DISABLE_EXCEPTION_CATCHING=0 -s DEMANGLE_SUPPORT=1
#COPT += -s ASSERTIONS=0 -s DISABLE_EXCEPTION_CATCHING=1 -s DEMANGLE_SUPPORT=0

COPT += -Oz -g0 -s FORCE_FILESYSTEM=1
COPT += -s LZ4=0 --memory-init-file 0
COPT += -s TOTAL_MEMORY=512MB -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=0 -s TOTAL_STACK=16777216


WASM_FLAGS=-s BINARYEN_ASYNC_COMPILATION=1 -s WASM=1 -s BINARYEN_TRAP_MODE=\"clamp\"

ifdef ASYNC
	WASM_FLAGS += -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 -s 'EMTERPRETIFY_FILE="micropython.binary"' 
endif


LINK_FLAGS=-s MAIN_MODULE=1
THR_FLAGS=-s FETCH=1 -s USE_PTHREADS=0

check:
	$(ECHO) EMSDK=$(EMSDK)
	$(ECHO) EMSCRIPTEN=$(EMSCRIPTEN)	
	$(ECHO) EMSDK_NODE=$(EMSDK_NODE)
	$(ECHO) EMSCRIPTEN_TOOLS=$(EMSCRIPTEN_TOOLS)	
	$(ECHO) EM_CONFIG=$(EM_CONFIG)		
	$(ECHO) EMMAKEN_COMPILER=$(EMMAKEN_COMPILER)
	$(ECHO) EMMAKEN_CFLAGS=$(EMMAKEN_CFLAGS)
	$(ECHO) EM_CACHE=$(EM_CACHE)	
	$(ECHO) EM_CONFIG=$(EM_CONFIG)	
	$(ECHO) CPPFLAGS=$(CPPFLAGS)
	$(shell env|grep ^EM)
	$(ECHO) "Using [$(CPP)] as prepro"

	
interpreter:
	$(ECHO) "Building static executable $@"
	$(CC) $(CFLAGS) -g -o build/main.o main.c
	$(Q)$(CC) $(INC) $(COPT) $(WASM_FLAGS) $(LINK_FLAGS) $(THR_FLAGS) \
 -o $@ main.c $(LIBMICROPYTHON) \
 --preload-file assets@/assets --preload-file micropython/lib@/lib
	$(shell mv $(BASENAME).* $(BASENAME)/)

$(PROG): libs
	$(ECHO) "Building static executable $@"
	$(CC) $(CFLAGS) -g0 -o build/main.o main.c
	$(Q)$(CC) $(INC) $(COPT) $(WASM_FLAGS) $(LINK_FLAGS) $(THR_FLAGS) \
 -o $@ main.c $(LIBMICROPYTHON) \
 --preload-file assets@/assets --preload-file micropython/lib@/lib
	$(shell mv $(BASENAME).* $(BASENAME)/)



#


