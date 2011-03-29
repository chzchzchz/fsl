#ifndef WPKTINST_H
#define WPKTINST_H

#include <string>

class WritePkt;

class WritePktInstance
{
public:
	virtual ~WritePktInstance(void) { delete exprs; }
	const std::string& getFuncName(void) const { return funcname; }
	const WritePkt* getParent(void) const { return parent; }
	unsigned int getParamBufEntries(void) const;
	void genCode(const ArgsList* args_in = NULL) const;
	void genProto(void) const;
	void genExterns(TableGen* tg) const;
	void genTableInstance(TableGen* tg) const;
private:
	friend class WritePkt;	/* only writepkt may create this object */

	WritePktInstance(
		const WritePkt* in_parent,
		const class Type*,
		const std::string& fname,
		const ExprList*	exprs);

	const WritePkt		*parent;
	const class Type	*t;
	std::string		funcname;
	ExprList		*exprs;
};

#endif
