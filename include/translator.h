#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stdint.h>

int translate(const char* infile, unsigned int load_addr,
	      unsigned int zero_page, unsigned int page_size,
	      int(*op)(unsigned int, const uint8_t*, void*),
	      void *data);

#endif
