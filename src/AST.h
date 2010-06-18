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

	void print(std::ostream& out) const { out << "CONST VAR"; }
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
	EnumEnt(Id* in_id) : id(in_id) { assert (id != NULL); } 
	EnumEnt(Id* in_id, Number* in_num) : id(in_id), num(in_num)
	{ assert (id != NULL); }
	virtual ~EnumEnt() 
	{
		delete id;
		if (num != NULL) delete num;
	}
private:
	Id*	id;
	Number*	num;
};


class Enum : public GlobalStmt, public PtrList<EnumEnt>
{
public:
	Enum() {}
	Enum(Id* id) : name(id)  { assert (name != NULL); }
	void setName(Id* id)
	{
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

	void add(const Id* type_name, const Id* arg_name) 
	{
		types.push_back(type_name);
		names.push_back(arg_name);
	}

	unsigned int size(void) const { return types.size(); }
private:
	std::vector<const Id*>	types;
	std::vector<const Id*>	names;

};

class FuncCond : public CondExpr 
{
public: 
	FuncCond(const FCall* fc) {} 
	virtual ~FuncCond() {} 
};

std::ostream& operator<<(std::ostream& in, const GlobalStmt& gs);


#endif
