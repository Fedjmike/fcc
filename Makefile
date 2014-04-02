# CONFIG = [ debug | release | profiling ]
CONFIG ?= debug

CC ?= gcc
CFLAGS = -std=c11 -Werror -Wall -Wextra

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

#
# Build
#

HEADERS = $(wildcard inc/*.h)
OBJS = $(patsubst src/%.c, obj/$(CONFIG)/%.o, $(wildcard src/*.c))
OUT = fcc

OBJ = obj/$(CONFIG)
BIN = bin/$(CONFIG)

FCC = $(BIN)/$(OUT)

all: $(BIN) $(OBJ) $(FCC)

$(OBJ):
	mkdir -p $(OBJ)

$(OBJ)/%.o: src/%.c $(HEADERS)
	@echo " [CC] $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BIN):
	mkdir -p $(BIN)
	
$(FCC): $(OBJS)
	@echo " [LD] $@"
	@$(CC) $(LDFLAGS) $(OBJS) -o $@
	@du -hs $@*
	
clean:
	rm -rf obj/*/*.o
	rm -f bin/*/$(OUT)*
	
print:
	@echo "===================="
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

SILENT = >/dev/null
TESTS = tests/xor-list tests/hashset

tests-all: $(TESTS)
	
tests/%: tests/%.c $(FCC)
	@echo " [FCC] $@"
	@$(FCC) -I tests/include $< $(SILENT)
	@echo " [$@]"
	@$@ $(SILENT)

#	
#
#
	
.PHONY: all clean print test
.SUFFIXES: