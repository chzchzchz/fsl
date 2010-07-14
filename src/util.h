#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <sstream>

static inline std::string int_to_string(int i)
{
	std::string s;
	std::stringstream out;
	out << i;
	return out.str();
}

#endif
