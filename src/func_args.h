#ifndef FUNCARGS_H
#define FUNCARGS_H

#include <string>
#include <map>

typedef std::map<std::string, std::string> funcarg_map;

class FuncArgs
{
public:
	FuncArgs(const class ArgsList* args);
	virtual ~FuncArgs();
	bool lookupType(const std::string& name, std::string& field) const;
	const class Type* getType(const std::string& name) const;
	bool hasField(const std::string& name) const;
	const ArgsList* getArgsList(void) const { return args; }
private:
	ArgsList	*args;
	funcarg_map	args_map;
};

#endif
