#ifndef TRANSLATOR_H
#define TRANSLATOR_H

int translate(const char* filename, unsigned int load_addr,
	      unsigned int zero_page, unsigned int page_size);

#endif
