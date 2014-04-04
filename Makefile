# OS = [ linux | windows ]
OS ?= linux

ifeq ($(OS),windows)
	OS = osWindows
else
	OS = osLinux
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
CFLAGS = -std=c11 -Werror -Wall -Wextra
CFLAGS += -include defaults.h

ifeq ($(CONFIG),debug)
	CFLAGS += -DFCC_DEBUGMODE -g
	LDFLAGS += -g
endif
ifeq ($(CONFIG),release)
	CFLAGS += -O3
endif
ifeq ($(CONFIG),profiling)
	CFLAGS += -O3 -pg
endif

BINNAME = fcc
FCC ?= bin/$(CONFIG)/$(BINNAME)

SILENT = >/dev/null
POSTBUILD = @[ -e $@ ] && du -hs $@; [ -e $@.exe ] && du -hs $@.exe; echo

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
	@OS=$(OS) WORDSIZE=$(WORDSIZE) bash makedefaults.sh $< >$@

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

tests-all: $(TESTS)
	
bin/tests/%-error.txt: tests/%-error.c $(FCC)
	@echo " [$(FCC)] $@"
	@$(FCC) $(TFLAGS) $< >$@; [ $$? -eq 1 ]
	$(POSTBUILD)
	
bin/tests/%: tests/%.c $(FCC)
	@mkdir -p bin/tests
	@echo " [$(FCC)] $@"
	@$(FCC) $(TFLAGS) $< -o $@
	
	@echo " [$@]"
	@$@ $(SILENT)
	$(POSTBUILD)
	
print-tests:
	@echo "===================="
	@echo " FCC   : $(FCC)"
	@echo " TFLAGS: $(TFLAGS)"
	@echo " TESTS : $(TESTS)"
	@echo "===================="
	
#
# Partial selfbuild
#

selfbuild: bin/self/fcc

bin/self/fcc: $(OUT)
	@echo " [FCC+CC] fcc"
	@CC=$(CC) CFLAGS="$(CFLAGS)" CONFIG=$(CONFIG) bash selfbuild.sh
	$(POSTBUILD)
	
#	
#
#
	
.PHONY: all clean print print-tests tests-all selfbuild
.SUFFIXES: