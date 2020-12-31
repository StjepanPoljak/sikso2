proj = sikso2

ifeq ($(CC),cc)
	CC = gcc
endif

objs = $(patsubst %.c,%.o,$(wildcard *.c))
ifeq (,$(wildcard cpu6502-opcodes.c))
objs += cpu6502-opcodes.o
endif

CFLAGS := -o3 -DDEVICE_SAFEGUARD=100 -DDEVICE_TRACE -g -DCPU_TRACE

$(proj): $(objs)
	$(CC) $(CFLAGS) $(objs) -o $@ -lpthread

cpu6502-opcodes.c:
	./scripts/genops.py > $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY=run
run: $(proj)
	./$(proj)

.PHONY=clean
clean:
	rm -rf $(proj) *.o cpu6502-opcodes.c
