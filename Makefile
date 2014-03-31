# CONFIG = [ debug | release | profiling ]
CONFIG ?= debug

CC = gcc
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

HEADERS = $(wildcard inc/*.h)
OBJS = $(patsubst src/%.c, obj/$(CONFIG)/%.o, $(wildcard src/*.c))
OUT = fcc

all: bin/$(CONFIG)/$(OUT)

obj/$(CONFIG):
	mkdir -p obj/$(CONFIG)

obj/$(CONFIG)/%.o: src/%.c $(HEADERS) obj/$(CONFIG)
	@echo " [CC] $@"
	@$(CC) $(CFLAGS) -c $< -o $@

bin/$(CONFIG):
	mkdir -p bin/$(CONFIG)
	
bin/$(CONFIG)/$(OUT): $(OBJS) bin/$(CONFIG)
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
	
.PHONY: all clean print
.SUFFIXES: