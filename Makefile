include ../../py/mkenv.mk

BASENAME=micropython
PROG=$(BASENAME).html
LIBMICROPYTHON = lib$(BASENAME).a
FROZEN_MPY_DIR = modules
FROZEN_DIR = assets

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

COPT=-s ASSERTIONS=2 -s ENVIRONMENT=web
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
LDFLAGS += -s EXPORTED_FUNCTIONS="['_main', '_getf','_setf', '_Py_InitializeEx', '_PyRun_SimpleString', '_PyRun_VerySimpleFile']"


SRC_C = \
	main.c \
	wasm_mphal.c \
	file.c \
	modos.c \
	modtime.c \
	moduos_vfs.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/mp-readline/readline.c \

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C) $(LIB_SRC_C)
# Append any auto-generated sources that are needed by sources listed in
# SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

SRC_ALL = $(SRC_C)
#SRC_ALL = $(SRC_C)


OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_ALL:.c=.o))

all: $(PROG)


#$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
#	$(ECHO) "MISC freezing bytecode"
#	../../tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@


include ../../py/mkrules.mk

clean:
	$(RM) -rf $(BUILD) $(CLEAN_EXTRA) $(LIBMICROPYTHON)

LIBMICROPYTHON = lib$(BASENAME)$(TARGET).a

#one day, maybe go via a *embeddable* static lib first  ?

$(LIBMICROPYTHON): $(OBJ)
	$(ECHO) "LIB $(LIBMICROPYTHON)"
	$(Q)$(AR) rcs $(LIBMICROPYTHON) $(OBJ)


EMOPTS = -s MAIN_MODULE=1  -Oz -g0  -s FORCE_FILESYSTEM=1 --memory-init-file 0
EMOPTS += -s TOTAL_MEMORY=512MB -s NO_EXIT_RUNTIME=1 -s ALLOW_MEMORY_GROWTH=0 -s TOTAL_STACK=16777216

$(PROG): static-lib
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(COPT) $(EMOPTS) -o $@ $(LIBMICROPYTHON) --preload-file data@/data
	$(shell mv $(BASENAME).* $(BASENAME)/)

#


