#include <stdlib.h> /* strtol */
#include <errno.h>

int parse_str(const char* str, void(*on_err)(int)) {
	int ret;

	errno = 0;
	ret = (int)strtol(str, NULL, 16);

	if (errno && on_err) {
		on_err(errno);

		return -1;
	}

	return ret;
}

