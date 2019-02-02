include ../../py/mkenv.mk

BASENAME=micropython
PROG=$(BASENAME).html
LIBMICROPYTHON = lib$(BASENAME).a
FROZEN_MPY_DIR = modules
FROZEN_DIR = flash

# clang has slightly different options to GCC
CLANG = 1
EMSCRIPTEN = 1

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include ../../py/py.mk

CC = emcc
CPP = gcc -E

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

#CFLAGS += -D MICROPY_NLR_SETJMP=1
#CFLAGS += -D MICROPY_USE_INTERNAL_PRINTF=0

# //for qstr.c
CFLAGS +=-DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool

# //for build/genhdr/qstr.i.last
QSTR_GEN_EXTRA_CFLAGS=-DMICROPY_QSTR_EXTRA_POOL=mp_qstr_frozen_const_pool

MPY_CROSS_FLAGS += -mcache-lookup-bc

LD = $(CC)
LDFLAGS = -Wl,-map,$@.map -Wl,-dead_strip -Wl,-no_pie
LDFLAGS += -s EXPORTED_FUNCTIONS="['_main', '_Py_InitializeEx', '_PyRun_SimpleString', '_PyRun_VerySimpleFile']"

SRC_C = \
	main.c \
	wasm_mphal.c \
	file.c \
	modos.c \
	modffi.c \
	modtime.c \
	moduos_vfs.c \
	ffi/types.c \
	ffi/prep_cif.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/mp-readline/readline.c \

SRC_MOD+=modembed.c


# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(LIB_SRC_C)

# Append any auto-generated sources that are needed by sources listed in
# SRC_QSTR
SRC_QSTR_AUTO_DEPS +=


OBJ = $(PY_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))
OBJ += $(addprefix $(BUILD)/, $(SRC_MOD:.c=.o))


all: $(PROG)


include ../../py/mkrules.mk

clean:
	$(RM) -rf $(BUILD) $(CLEAN_EXTRA) $(LIBMICROPYTHON)

LIBMICROPYTHON = lib$(BASENAME)$(TARGET).a

#one day, maybe go via a *embeddable* static lib first  ?

$(LIBMICROPYTHON): $(OBJ)
	$(ECHO) "LIB $(LIBMICROPYTHON)"
	$(Q)$(AR) rcs $(LIBMICROPYTHON) $(OBJ)


# EMOPTS+= -Oz -g0 -s FORCE_FILESYSTEM=1  --memory-init-file 1


COPT=-s ENVIRONMENT=web
# https://github.com/emscripten-core/emscripten/wiki/Linking
COPT+= -s MAIN_MODULE=1

#see https://github.com/emscripten-core/emscripten/issues/7811
#COPT+= -s EXPORT_ALL=1

COPT += -s ASSERTIONS=2 -s DISABLE_EXCEPTION_CATCHING=0 -s DEMANGLE_SUPPORT=1
#COPT += -s ASSERTIONS=0 -s DISABLE_EXCEPTION_CATCHING=1 -s DEMANGLE_SUPPORT=0
COPT += -Oz -g0 -s FORCE_FILESYSTEM=1
COPT += -s LZ4=0 --memory-init-file 0
COPT += -s TOTAL_MEMORY=512MB -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=0 -s TOTAL_STACK=16777216


$(PROG): static-lib
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(COPT) -o $@ $(LIBMICROPYTHON) -ldl --preload-file assets@/assets --preload-file micropython/lib@/lib
	$(shell mv $(BASENAME).* $(BASENAME)/)

#


