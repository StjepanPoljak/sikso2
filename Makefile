proj = sikso2

ifeq ($(CC),cc)
	CC = gcc
endif

objs = $(subst source,build,$(patsubst %.c,%.o,$(wildcard source/*.c)))
ifeq (,$(wildcard source/cpu6502-opcodes.c))
objs += build/cpu6502-opcodes.o
endif

CONFIG_FILE ?= config.txt

CFLAGS := -Iinclude -o3 -g -Wall
OPTIONS = $(shell sed '/^\s*\#/d' $(CONFIG_FILE) | awk ' \
	BEGIN { opts="" } \
	{ opts = opts (opts == "" ? "" : OFS) "-D" $$0 } \
	END { print opts }')

CFLAGS += $(OPTIONS)

common := config.txt

all: $(proj) build

.PHONY=build
build:
	mkdir build

$(proj): build $(objs) $(common)
	$(CC) $(CFLAGS) $(objs) -o $@ -lpthread

source/cpu6502-opcodes.c: $(common)
	./scripts/genops.py > $@

build/%.o: source/%.c $(common)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY=run
run: $(proj)
	./$(proj)

.PHONY=clean
clean:
	rm -rf $(proj) build source/cpu6502-opcodes.c
