#ifndef FUNC_H
#define FUNC_H

#include <string>
#include <map>
#include <list>

#include "collection.h"
#include "expr.h"

extern int yylineno;

typedef std::map<std::string, class Func*>	func_map;
typedef std::list<class Func*>			func_list;

class FuncStmt 
{
public:
	virtual ~FuncStmt() {}
	unsigned int getLineNo() { return lineno; }
protected:
	FuncStmt() { lineno = yylineno; }
private:
	unsigned int lineno;
};

class FuncBlock : public FuncStmt, public PtrList<FuncStmt>
{
public:
	FuncBlock() {}
	virtual ~FuncBlock() {}
private:
};

class FuncCondStmt : public FuncStmt
{
public:
	FuncCondStmt(CondExpr* ce, FuncStmt* taken, FuncStmt* not_taken)
	: cond(ce), is_true(taken), is_false(not_taken)
	{
		assert (cond != NULL);
		assert (is_true != NULL);
	}

	virtual ~FuncCondStmt() 
	{
		delete cond;
		delete is_true;
		if (is_false != NULL) delete is_false;
	}

private:
	CondExpr	*cond;
	FuncStmt	*is_true;
	FuncStmt	*is_false;
};

class FuncDecl : public FuncStmt
{ 
public:
	FuncDecl(Id* in_type, Id* in_name) 
	: type(in_type), scalar(in_name), array(NULL)
	{
		assert (type != NULL);
		assert (scalar != NULL);
	}
	FuncDecl(Id* in_type, IdArray* in_name) 
	: type(in_type), scalar(NULL), array(in_name)
	{
		assert (type != NULL);
		assert (array != NULL);
	} 

	virtual ~FuncDecl()
	{
		delete type;
		if (scalar != NULL) delete scalar;
		if (array != NULL) delete array;
	}
private:
	Id	*type;
	Id	*scalar;
	IdArray	*array;
};

class Func : public GlobalStmt 
{
public:
	Func(Id* in_ret_type, Id* in_name, ArgsList* in_al, FuncBlock* in_block)
	: 	ret_type(in_ret_type),
		name(in_name),
		args(in_al),
		block(in_block)
	{
		assert (ret_type != NULL);
		assert (name != NULL);
		assert (args != NULL);
		assert (block != NULL);
	}

	virtual ~Func() 
	{
		delete ret_type;
		delete name;
		delete args;
		delete block;
	}

	void print(std::ostream& out) const { out << "Func"; }

	const std::string& getName() const { return name->getName(); }
	const std::string& getRet() const { return ret_type->getName(); }
private:
	Id		*ret_type;
	Id		*name;
	ArgsList	*args;
	FuncBlock	*block;
};


#endif
