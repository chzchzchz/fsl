#ifndef ARGS_H
#define ARGS_H

typedef std::map<std::string, std::string> arg_map;

class ArgsList
{
public:
	ArgsList() {}
	virtual ~ArgsList();

	void add(Id* type_name, Id* arg_name);
	unsigned int size(void) const { return types.size(); }
	std::pair<Id*, Id*> get(unsigned int i) const;

	bool lookupType(const std::string& name, std::string& type_ret) const;
	const class Type* getType(const std::string& name) const;
	bool hasField(const std::string& name) const;
	ArgsList* copy(void) const;

private:
	std::vector<Id*>			types;
	std::vector<Id*>			names;
	arg_map					name_to_type;

};


#endif
