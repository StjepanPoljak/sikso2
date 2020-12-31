proj = sikso2

ifeq ($(CC),cc)
	CC = gcc
endif

objs = $(subst source,build,$(patsubst %.c,%.o,$(wildcard source/*.c)))
ifeq (,$(wildcard source/cpu6502-opcodes.c))
objs += build/cpu6502-opcodes.o
endif

CFLAGS := -Iinclude -o3 -DDEVICE_SAFEGUARD=100 -DDEVICE_TRACE -g -DCPU_TRACE

all: $(proj) build

.PHONY=build
build:
	mkdir build

$(proj): build $(objs)
	$(CC) $(CFLAGS) $(objs) -o $@ -lpthread

source/cpu6502-opcodes.c:
	./scripts/genops.py > $@

build/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY=run
run: $(proj)
	./$(proj)

.PHONY=clean
clean:
	rm -rf $(proj) build source/cpu6502-opcodes.c
