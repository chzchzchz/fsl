#ifndef TOKEN_H
#define TOKEN_H

#include <assert.h>

static unsigned long getLongHex(const char* s)
{
	unsigned long 	ret;
	int		i;

	ret = 0;
	for (i = 0; s[i]; i++) {
		char	c = s[i];
		ret <<= 4;
		if (c >= 'A' && c <= 'F')
			c -= ('a' - 'A');
		ret |= (c >= 'a') ? (c - 'a') + 10 : c - '0';
	}
	return ret;
}

#endif
