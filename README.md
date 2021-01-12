# sikso2

## Introduction

Sikso2, a 6502 CPU translator and emulator, still in development.

## Configuration

File `config.txt` contains build-time configuration options for sikso2. Mandatory options are:

```
DEFAULT_LOAD_ADDR=0x0600
DEFAULT_ZERO_PAGE=0x0
DEFAULT_PAGE_SIZE=256
DEFAULT_DUMP_MEM_COLS=5
```

You can, of course, change these to your preference. Note that some of them are still not fully functional, such as `DEFAULT_ZERO_PAGE`, as emulator still does run-time checks for `0x00` to `0xFF` address range in certain places.

Debug and trace options that can be removed include:

```
MAIN_TRACE
CPU_TRACE
DEVICE_TRACE
TRANSLATOR_TRACE
SAFEGUARD=1000
```

Safeguard is a convenient option to avoid an infinite loop when running the device. In the future, this will be replaced by a `SIGINT` trap that will do a proper device cleanup.

Note: You can turn off any of these options by either deleting them or, preferrably, commenting them out with `#`, i.e. the hashtag.

## Build

To build the program, simply use `make`.

## Translation

The assembler follows a rather simple syntax and is almost fully functional. You can try:

```shell
./sikso2 -t test.asm -o test.bin
```

This will create a binary that is executable on a 6502 CPU.

## Run

### Run binary

To run binary (e.g. as obtained by translating), simply use:

```shell
./sikso2 -R <binary_file>
```

### Run assembly file

The best way to run an assembly file is to use:

```shell
./sikso2 -s -r test.asm
```

This will translate `test.asm` to binary and then run it in the emulator. The `-s` option will stop the emulation when the last instruction is reached. Needless to say, this is problematic if an infinite loop is involved. But, it is useful for debug and simple programs.

## Dumps

You can see the state of CPU registers when execution stops:

```shell
./sikso2 -s -r test.asm -d pretty
```

Also, you can print out memory ranges:

```shell
./sikso2 -s -r test.asm -m 0x0600-0x060a,0x0700
```

This command will print out memory from `0x0600` to (and including) `0x060a`, but also a byte on `0x0700`.

## Memory

### Load image

You can also manipulate memory that will be used in the device. To load an image to RAM, use:

```shell
./sikso2 -s -r test.asm -f 0x0700:<file>
```

This will load contents of `<file>` to RAM at address `0x0700`.

### Load bytes

You can also load byte-by-byte to RAM, using the following syntax:

```shell
./sikso2 -s -r test.asm -b 0x0700:0e,0x0702:ff
```

This will load byte `0x0e` at address `0x0700` and byte `0xff` at address `0x0702` in RAM.

### Load order

Keep in mind that sikso2 will first load the RAM image, then binary, and then bytes. So, you can overwrite loaded RAM image with binary, and tweak both image and binary with single bytes.

## Help

For further CPU dump options and other switches, use:

```shell
./sikso2 -h
```
