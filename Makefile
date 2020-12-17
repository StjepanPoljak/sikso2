proj = gitus

ifeq ($(CC),cc)
	CC = gcc
endif

objs = $(patsubst %.c,%.o,$(wildcard *.c))
ifeq (,$(wildcard cpu6502-opcodes.c))
objs += cpu6502-opcodes.o
endif

$(proj): $(objs)
	$(CC) $(objs) -o $@ -lpthread

cpu6502-opcodes.c:
	./scripts/genops.py > $@

%.o: %.c
	$(CC) -c $< -o $@

.PHONY=run
run: $(proj)
	./$(proj)

.PHONY=clean
clean:
	rm -rf $(proj) *.o cpu6502-opcodes.c
