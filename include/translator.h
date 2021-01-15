#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stdint.h>

int translate(const char* infile, unsigned int load_addr,
	      int(*op)(unsigned int, const uint8_t*, void*),
	      void *data);

#endif
