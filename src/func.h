#ifndef FUNC_H
#define FUNC_H
#include "expr.h"

class FuncStmt 
{
public:
	FuncStmt() {}
	virtual ~FuncStmt() {}
};

class FuncBlock : public FuncStmt 
{
public:
	FuncBlock() {}
	virtual ~FuncBlock() {}
	void add(const FuncStmt* stmt) {}
};

class FuncCondStmt : public FuncStmt
{
public:
	FuncCondStmt(
		const CondExpr*, const FuncStmt* taken, const FuncStmt* ow)
	{}
	virtual ~FuncCondStmt() {}
};

class FuncDecl : public FuncStmt
{ 
public:
	FuncDecl(const Id* type, const Id* name) {}
	FuncDecl(const Id* type, const IdArray* name) {} 
};

class Func : public GlobalStmt 
{
public:
	Func(	const Id* ret, const Id* name,
		const ArgsList* al, const FuncBlock* block)
	{
		/* XXX */
	}

	virtual ~Func() {}

	void print(std::ostream& out) const { out << "Func"; }
};


#endif
