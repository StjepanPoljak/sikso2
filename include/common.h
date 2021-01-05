#ifndef COMMON_H
#define COMMON_H

#define log_err(SIG, FMT, ...) printf("[" SIG "] (!) " FMT "\n", ## __VA_ARGS__)
#define trace(FMT, ...) printf("  -> " FMT "\n", ## __VA_ARGS__)
#define tracei(SIG, FMT, ...) printf("[" SIG "] (i) " FMT "\n", ## __VA_ARGS__)

int parse_str(const char* str, int base, void(*on_err)(int));

#endif
