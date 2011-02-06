#ifndef ARGS_H
#define ARGS_H

#include <map>
#include <string>
#include <list>

class Id;

typedef std::map<std::string, std::string>	arg_map;
typedef std::pair<Id*, Id*>			arg_elem;

#define argelem_type(x)	((x).first)->getName()
#define argelem_name(x) ((x).second)->getName()

class ArgsList
{
public:
	ArgsList() {}
	virtual ~ArgsList();

	void add(Id* type_name, Id* arg_name);
	unsigned int size(void) const { return types.size(); }
	arg_elem get(unsigned int i) const;
	void clear();

	int find(const std::string& name) const;
	int getParamBufBaseIdx(int idx) const;
	bool lookupType(const std::string& name, std::string& type_ret) const;
	const class Type* getType(const std::string& name) const;
	bool hasField(const std::string& name) const;
	ArgsList* copy(void) const;
	unsigned int getNumParamBufEntries(void) const;
	ArgsList* rebind(const std::list<const Id*> &ids) const;
private:
	std::vector<Id*>			types;
	std::vector<Id*>			names;
	arg_map					name_to_type;

};

#endif
