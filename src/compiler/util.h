#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>

#define iter_do(x,y,z)	\
	do {	\
		for (	x::const_iterator it = (y).begin(); 	\
			it != (y).end();			\
			it++) {					\
				(*it)->z();			\
			}					\
	} while (0)

static inline std::string int_to_string(int i)
{
	std::string s;
	std::stringstream out;
	out << i;
	return out.str();
}

static inline unsigned long getLongHex(const char* s)
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
