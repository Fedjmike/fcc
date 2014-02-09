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
OUT = bin/$(CONFIG)/fcc

obj/$(CONFIG)/%.o: src/%.c $(HEADERS)
	@echo " [CC] $@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJS)
	@echo " [LD] $(OUT)"
	@$(CC) $(LDFLAGS) $(OBJS) -o $(OUT)
	@du -hs bin/$(CONFIG)/fcc*
	
clean:
	rm -rf obj/*/*.o
	rm -f bin/*/fcc*
	
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
