#ifndef AST_H
#define AST_H

#include <iostream>
#include <vector>
#include <list>
#include <string>
#include "expr.h"

class GlobalStmt 
{
public:
	virtual ~GlobalStmt() {}
protected:
	GlobalStmt() {}
};


class Scope {
public:
	Scope() {} 
	virtual ~Scope() 
	{
		std::list<GlobalStmt*>::iterator	it, end;

		end = stmts.end();
		for (it = stmts.begin(); it != end; it++) {
			delete (*it);
		}
	}

	void add(GlobalStmt* s)
	{
		stmts.push_back(s);
	}

	unsigned int getStmts(void) const
	{
		return stmts.size();
	}

private:
	std::list<GlobalStmt*>	stmts;
};



class ConstVar : public GlobalStmt
{
public:
	ConstVar(const Id* in_id, const Expr* e) :
		id(in_id),
		expr(e)
	{
	}

	virtual ~ConstVar() {}
private:
	const Id*		id;
	const Expr*		expr;
};

class ConstArray : public GlobalStmt
{
public:
	ConstArray(const Id* in_id, const ExprList* e) :
		id(in_id),
		elist(e)
	{
	}

	virtual ~ConstArray() {}
private:
	const Id*		id;
	const ExprList*		elist;
};


class CondExpr 
{
public:
	virtual ~CondExpr() {}
protected:
	CondExpr() {} 
};


class BinBoolOp : public CondExpr 
{
public:
	virtual ~BinBoolOp() {}
	BinBoolOp(CondExpr*, CondExpr*) {}
};

class BOPAnd : public BinBoolOp 
{
public:
	BOPAnd(CondExpr* lhs, CondExpr* rhs) : BinBoolOp(lhs, rhs) {} 
	virtual ~BOPAnd() {} 
};

class BOPOr : public BinBoolOp
{
public:
	BOPOr(CondExpr* lhs, CondExpr* rhs) : BinBoolOp(lhs, rhs) {} 
	virtual ~BOPOr() {} 
};

class CmpOp : public CondExpr {
public:
	CmpOp(Expr* lhs, Expr* rhs) {}
	virtual ~CmpOp() {} 
private:
	CmpOp() {}
};

class CmpEQ : public CmpOp 
{
public:
	CmpEQ(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpEQ() {} 
};

class CmpNE : public CmpOp 
{
public:
	CmpNE(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpNE() {} 
};

class CmpLE : public CmpOp
{
public:
	CmpLE(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpLE() {} 
};

class CmpGE : public CmpOp
{
public:
	CmpGE(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpGE() {} 
};

class CmpLT : public CmpOp
{
public:
	CmpLT(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpLT() {} 
};

class CmpGT : public CmpOp
{
public:
	CmpGT(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpGT() {} 
};


class UnArithOp {
};

class UnBoolOp {
};

class EnumEnt 
{
public:
	EnumEnt(const Id* id) {} 
	EnumEnt(const Id* id, const Number* num) {}
	~EnumEnt() {}
};


class Enum : public GlobalStmt 
{
public:
	Enum() {}
	Enum(const Id* id) {} 
	void add(const EnumEnt* ent) {}
	void setName(const Id* id) {}
	virtual ~Enum() {} 
private:
};

class FuncBlock {
};

class FuncCond : public CondExpr {
public: 
	FuncCond(const FCall* fc) {} 
	virtual ~FuncCond() {} 
};

#endif
