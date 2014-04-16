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
CFLAGS += -Werror -Wall -Wextra
CFLAGS += -include defaults.h

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
OBJS = $(patsubst src/%.c, obj/$(CONFIG)/%.o, $(wildcard src/*.c))

OBJ = obj/$(CONFIG)
BIN = bin/$(CONFIG)

OUT = $(BIN)/$(BINNAME)

all: $(OUT)

defaults.h: defaults.in.h
	@echo " [makedefaults.h] $@"
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

TFLAGS = -I tests/include
TESTS = $(patsubst %, bin/tests/%, xor-list hashset xor-list-error.txt)

ifneq ($(shell command -v valgrind; echo $?),)
	VFLAGS = -q --leak-check=full --workaround-gcc296-bugs=yes
	VALGRIND ?= valgrind $(VFLAGS)
endif

tests-all: $(TESTS)
	
bin/tests/%-error.txt: tests/%-error.c $(FCC)
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
# Partial selfbuild
#

selfbuild: bin/self/fcc

bin/self/fcc: $(OUT) selfbuild.sh
	@echo " [FCC+CC] fcc"
	@CFLAGS="$(CFLAGS)" CONFIG=$(CONFIG) bash selfbuild.sh
	$(POSTBUILD)
	
#	
#
#
	
.PHONY: all clean print print-tests tests-all selfbuild
.SUFFIXES: