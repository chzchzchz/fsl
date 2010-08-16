#ifndef TOKEN_H
#define TOKEN_H

#include <assert.h>

static unsigned long getLongHex(const char* s)
{
	unsigned long 	ret;
	int		i;

	ret = 0;
	for (i = 0; s[i]; i++) {
		unsigned char	c = s[i];

		ret <<= 4;
		if (c >= 'A' && c <= 'F') {
			c -= 'A';
			c += 'a';
		}

		if (c >= 'a' && c <= 'f')
			ret |= (c - 'a') + 10;
		else
			ret |= c - '0';
	}

	return ret;
}

#endif
