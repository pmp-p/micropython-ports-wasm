include ../../py/mkenv.mk

BASENAME=micropython
PROG=$(BASENAME).html
LIBMICROPYTHON = lib$(BASENAME).a


# clang has slightly different options to GCC
CLANG = 1
EMSCRIPTEN = 1



# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

# include py core make definitions
include ../../py/py.mk

CC = emcc -s ASSERTIONS=2 -s EMULATE_FUNCTION_POINTER_CASTS=1
CPP = gcc -E
CLANG = 1
SIZE = echo
LD = emcc -s ASSERTIONS=2 -s EMULATE_FUNCTION_POINTER_CASTS=1

INC += -I.
INC += -I../..
INC += -I$(BUILD)

CFLAGS = $(INC) -Wall -Werror -ansi -std=gnu99 $(COPT)

#Debugging/Optimization
ifeq ($(DEBUG), 1)
CFLAGS += -O0
CC += -g4
LD += -g4
else
CFLAGS += -Os -DNDEBUG
endif

CFLAGS += -D MICROPY_NLR_SETJMP=1
CFLAGS += -D MICROPY_USE_INTERNAL_PRINTF=0

LD = $(CC)
LDFLAGS = -Wl,-map,$@.map -Wl,-dead_strip -Wl,-no_pie
LDFLAGS += -s EXPORTED_FUNCTIONS="['_main', '_getf','_setf', '_Py_InitializeEx', '_PyRun_SimpleString', '_PyRun_VerySimpleFile']"


SRC_C = \
	main.c \
	stdio_core.c \
	lib/utils/stdout_helpers.c \
	lib/utils/pyexec.c \
	lib/mp-readline/readline.c \
	$(BUILD)/_frozen_mpy.c \

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

all: $(PROG)


$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)../../tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@


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

$(PROG): static-lib $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(EMOPTS) -o $@ $(LIBMICROPYTHON) --preload-file ./assets/boot.py --preload-file ./assets/main.py
	$(shell mv $(BASENAME).* $(BASENAME)/)


