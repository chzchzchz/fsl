#ifndef AST_H
#define AST_H

#include "collection.h"

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include "expr.h"

class GlobalStmt 
{
public:
	virtual ~GlobalStmt() {}
	virtual void print(std::ostream& out) const 
		{ out << "BASE CLASS"; }
protected:
	GlobalStmt() {}
};


class GlobalBlock : public PtrList<GlobalStmt>
{
public:
	GlobalBlock() {} 
	virtual ~GlobalBlock() {}
};



class ConstVar : public GlobalStmt
{
public:
	ConstVar(Id* in_id, Expr* e) :
		id(in_id),
		expr(e)
	{
		assert (id != NULL);
		assert (expr != NULL);
	}

	virtual ~ConstVar()
	{
		delete id;
		delete expr;
	}

	void print(std::ostream& out) const 
	{
		out << "CONST: "; 
		id->print(out);
		out << " = ";
		expr->print(out);
	}

	const std::string& getName(void) const { return id->getName(); }
	const Expr* getExpr(void) const { return expr; }
private:
	Id*		id;
	Expr*		expr;
};

class ConstArray : public GlobalStmt
{
public:
	ConstArray(Id* in_id, ExprList* e) :
		id(in_id),
		elist(e)
	{
		/* XXX */
		std::cerr << 
			"Ooops: Constant arrays not supported. Ignoring" << 
			id->getName() << std::endl;
		assert (e != NULL);
		assert (id != NULL);
	}

	virtual ~ConstArray() 
	{
		delete id;
		delete elist;
	}

	void print(std::ostream& out) const { out << "CONST ARRAY"; }
private:
	Id*			id;
	ExprList*		elist;
};


class UnArithOp {
};

class UnBoolOp {
};

class EnumEnt 
{
public:
	EnumEnt(Id* in_id) : id(in_id) { assert (id != NULL); } 
	EnumEnt(Id* in_id, Number* in_num) : id(in_id), num(in_num)
	{ assert (id != NULL); }
	virtual ~EnumEnt() 
	{
		delete id;
		if (num != NULL) delete num;
	}

	const Number* getNumber(void) const { return NULL; }
	const std::string& getName(void) const 
	{ 
		assert (id != NULL);
		return id->getName(); 
	}
private:
	Id*	id;
	Number*	num;
};


class Enum : public GlobalStmt, public PtrList<EnumEnt>
{
public:
	Enum() : name(NULL) {}
	Enum(Id* id) : name(id)  { assert (name != NULL); }
	void setName(Id* id)
	{
		assert (id != NULL);
		if (name != NULL) delete name; 
		name = id;
	}

	virtual ~Enum() 
	{
		if (name != NULL) delete name;
	} 

	void print(std::ostream& out) const { out << "ENUM"; }
private:
	Id	*name;
};



class ArgsList	 
{
public:
	ArgsList() {}
	~ArgsList() 
	{
		for (int i = 0; i < types.size(); i++) {
			delete types[i];
			delete names[i];
		}
	}

	void add(Id* type_name, Id* arg_name) 
	{
		types.push_back(type_name);
		names.push_back(arg_name);
	}

	unsigned int size(void) const { return types.size(); }

	std::pair<Id*, Id*> get(unsigned int i) const
	{
		return std::pair<Id*, Id*>(types[i], names[i]);
	}
private:
	std::vector<Id*>	types;
	std::vector<Id*>	names;

};

class FuncCond : public CondExpr 
{
public: 
	FuncCond(const FCall* fc) {} 
	virtual ~FuncCond() {} 
};

std::ostream& operator<<(std::ostream& in, const GlobalStmt& gs);

#endif
