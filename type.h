#ifndef TYPE_H
#define TYPE_H

#include "expr.h"

class TypeArgs
{
public:
	TypeArgs() {}
	~TypeArgs() 
	{
		for (int i = 0; i < types.size(); i++) {
			delete types[i];
			delete names[i];
		}
	}

	void add(const Id* type_name, const Id* arg_name) 
	{
		types.push_back(type_name);
		names.push_back(arg_name);
	}
private:
	std::vector<const Id*>	types;
	std::vector<const Id*>	names;

};

class TypeStmt
{
public:
	virtual ~TypeStmt() {}
protected:
	TypeStmt() {}
};

/* declaration */
class TypeDecl : public TypeStmt
{
public:
	TypeDecl(const Id* type, const Id* name) : TypeStmt()
	{
	}
	TypeDecl(const Id* type, const IdArray* array) : TypeStmt()
	{
	}
	virtual ~TypeDecl() {}
};


class TypeCond: public TypeStmt
{
public:
	TypeCond(CondExpr* ce, TypeStmt* taken) : TypeStmt() {}
	TypeCond(CondExpr* ce, TypeStmt* taken, TypeStmt* nottaken) 
		: TypeStmt() {}
};

class TypePreamble
{
public:
	TypePreamble() {}
	~TypePreamble() 
	{
		for (int i = 0; i < funcs.size(); i++)
			delete funcs[i];
	}

	void add(const FCall* func) { funcs.push_back(func); }
	const std::vector<const FCall*>&	get() const { return funcs; }
private:
	std::vector<const FCall*>	funcs;
};

class TypeBlock : public TypeStmt
{
public:
	TypeBlock() {}
	~TypeBlock() 
	{
		for (int i = 0; i < stmts.size(); i++)
			delete stmts[i];
	}

	void add(const TypeStmt* stmt) { stmts.push_back(stmt); }
	const std::vector<const TypeStmt*>&	get() const { return stmts; }
private:
	std::vector<const TypeStmt*>	stmts;
};

class TypeUnion : public TypeStmt
{
public:
	TypeUnion(const TypeBlock* tb, const Id* name)
	{
	}
	TypeUnion(const TypeBlock* tb, const IdArray* array)
	{
	}
};


class TypeFunc : public TypeStmt
{
public:
	TypeFunc(const FCall* fcall) {} 
	virtual ~TypeFunc() {}
};


class Type : public GlobalStmt 
{
public:
	Type(	const Id* name, const TypeArgs* ta, const TypePreamble* pl,
		const TypeBlock* tb)
	{
	}

};



#endif
