# OS = [ linux | windows ]
OS ?= linux

ifeq ($(OS),windows)
	OS_ = osWindows
else
	OS_ = osLinux
endif

# ARCH = [ 32 | 64 ]
ARCH ?= 32

ifeq ($(ARCH),64)
	WORDSIZE = 8
else
	WORDSIZE = 4
endif

# CONFIG = [ debug | release | profiling ]
CONFIG ?= release

CC ?= gcc
CFLAGS ?= -std=c11
CFLAGS += -Werror -Wall -Wextra -Wvla -Wstrict-aliasing -Wstrict-overflow=5 -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wmissing-field-initializers -g
CFLAGS += -include defaults.h

ifeq ($(STRICT),yes)
	CFLAGS += -Wformat=2 -Wmissing-include-dirs -Wconversion -pedantic
	CFLAGS += -Wno-format-nonliteral -Wno-sign-conversion
endif

ifeq ($(CONFIG),debug)
	CFLAGS += -DFCC_DEBUGMODE -g
	LDFLAGS += -g
endif
ifeq ($(CONFIG),release)
	CFLAGS += -O3
	LDFLAGS += -O3
endif
ifeq ($(CONFIG),profiling)
	CFLAGS += -O3 -pg
	LDFLAGS += -O3 -pg
endif
ifeq ($(CONFIG),coverage)
	CFLAGS += --coverage
	LDFLAGS += --coverage
endif

BINNAME = fcc
FCC ?= bin/$(CONFIG)/$(BINNAME)

SILENT = >/dev/null
POSTBUILD = @[ -e $@ ] && du -hs $@; echo

#
# Build
#

HEADERS = $(wildcard inc/*.h) defaults.h
OBJS = $(patsubst src/%.c, obj/$(CONFIG)/%.o, $(filter-out src/lexerself.c, $(filter-out src/regself.c, $(wildcard src/*.c))))

OBJ = obj/$(CONFIG)
BIN = bin/$(CONFIG)

OUT = $(BIN)/$(BINNAME)

all: $(OUT)

defaults.h: defaults.in.h
	@echo " [makedefaults.sh] $@"
	@OS=$(OS_) WORDSIZE=$(WORDSIZE) bash makedefaults.sh $< >$@

$(OBJ)/%.o: src/%.c $(HEADERS)
	@mkdir -p $(OBJ)
	@echo " [CC] $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJS)
	@mkdir -p $(BIN)
	@echo " [LD] $@"
	@$(CC) $(LDFLAGS) $(OBJS) -o $@
	$(POSTBUILD)

clean:
	rm -f defaults.h
	rm -f obj/*/*.o
	rm -f bin/*/$(BINNAME)*
	rm -f bin/tests/*

print:
	@echo "===================="
	@echo " OS     : $(OS)"
	@echo " ARCH   : $(ARCH)"
	@echo " CONFIG : $(CONFIG)"
	@echo " CC     : $(CC)"
	@echo " CLFAGS : $(CFLAGS)"
	@echo " HEADERS: $(HEADERS)"
	@echo " OBJS   : $(OBJS)"
	@echo " OUT    : $(OUT)"
	@echo "===================="

#
# Tests
#

TFLAGS = -I tests/include -s
TOUT = xor-list hashset xor-list-error.txt
TESTS = $(patsubst %, bin/tests/%, $(TOUT))

ifneq ($(shell command -v valgrind; echo $?),)
	VFLAGS = -q --leak-check=full --workaround-gcc296-bugs=yes --error-exitcode=1
	VALGRIND ?= valgrind $(VFLAGS)
endif

tests: $(TESTS)

bin/tests/%-error.txt: tests/%-error.c $(FCC)
	@mkdir -p bin/tests
	@echo " [$(FCC)] $@"
	@$(VALGRIND) $(FCC) $(TFLAGS) $< >$@; [ $$? -eq 1 ]
	$(POSTBUILD)

bin/tests/%: tests/%.c $(FCC)
	@mkdir -p bin/tests
	@echo " [$(FCC)] $@"
	@$(VALGRIND) $(FCC) $(TFLAGS) $< -o $@
	
	@echo " [$@]"
	@$@ $(SILENT)
	$(POSTBUILD)

print-tests:
	@echo "===================="
	@echo " FCC     : $(FCC)"
	@echo " TFLAGS  : $(TFLAGS)"
	@echo " TESTS   : $(TESTS)"
	@echo " VALGRIND: $(VALGRIND)"
	@echo "===================="
	
#
# Selfhost
#

selfhost: bin/self/fcc

bin/self/%: $(FCC) src/lexerself.c src/regself.c
	@echo " [$(FCC)] $@"
	@$(VALGRIND) $(FCC) -I tests/include `find src/*.[c] | egrep -v "lexer.c|reg.c"` -o $@
	$(POSTBUILD)

#
#
#

.PHONY: all clean print print-tests tests selfhost
.SUFFIXES:
