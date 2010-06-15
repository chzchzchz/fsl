#ifndef EXPR_H
#define EXPR_H

#include "collection.h"

class Expr
{
public:
	virtual void print(std::ostream& out) const = 0;
	virtual ~Expr() {}
protected:
	Expr() {}
};

class ExprList : public PtrList<Expr>
{
public:
	ExprList() {}
	virtual ~ExprList()  {}
	void print(std::ostream& out) const
	{
		const_iterator	it;

		it = begin();
		if (it == end()) return;

		(*it)->print(out);
		it++;
		for (; it != end(); it++) {
			out << ", ";
			(*it)->print(out);
		}
	}
private:
};

class Id  : public Expr
{
public: 
	Id(const std::string& s) : id_name(s) {}
	virtual ~Id() {}
	const std::string& getName() const { return id_name; }
	void print(std::ostream& out) const { out << id_name; }
private:
	const std::string id_name;
};


class FCall  : public Expr
{
public:
	FCall(Id* in_id, ExprList* in_exprs) 
	:	id(in_id),
		exprs(in_exprs)
	{ 
		assert (id != NULL);
		assert (exprs != NULL);
	}
	virtual ~FCall()
	{
		delete id;
		delete exprs;
	}
	ExprList* getExprs(void) const { return exprs; }
	std::string getName(void) const { return id->getName(); }
	void print(std::ostream& out) const
	{
		out << "fcall " << getName() << "("; 
		exprs->print(out);
		out << ")";
	}

private:
	Id*		id;
	ExprList*	exprs;
};

class IdStruct : public Expr, public PtrList<Expr>
{
public:
	IdStruct() { }
	virtual ~IdStruct() {} 
	void print(std::ostream& out) const
	{
		const_iterator	it;
		
		it = begin();
		assert (it != end());

		(*it)->print(out);

		for (; it != end(); it++) {
			out << '.';
			(*it)->print(out);
		}
	}
private:
};


class IdArray  : public Expr
{
public: 
	IdArray(Id *in_id, Expr* expr) : 
		id(in_id), idx(expr) 
	{
		assert (id != NULL);
		assert (idx != NULL);
	}

	virtual ~IdArray() 
	{
		delete id;
		delete idx;
	}

	const std::string& getName() const 
	{
		return id->getName();
	}

	void print(std::ostream& out) const 
	{
		id->print(out);
		out << '[';
		idx->print(out);
		out << ']';
	}

private:
	Id		*id;
	Expr		*idx;
};


class Number : public Expr
{
public:
	Number(unsigned long v) : n(v) {}
	virtual ~Number() {}

	unsigned long getValue() const { return n; }

	void print(std::ostream& out) const { out << n; }
private:
	unsigned long n;
};

class BinArithOp : public Expr
{
public:
	virtual ~BinArithOp() 
	{
		delete e_lhs;
		delete e_rhs;
	}

	void print(std::ostream& out) const
	{
		e_lhs->print(out);
		out << ' ' << getOpSymbol() << ' ';
		e_rhs->print(out);
	}

protected:
	BinArithOp(Expr* e1, Expr* e2)
		: e_lhs(e1), e_rhs(e2) 
	{
		assert (e_lhs != NULL);
		assert(e_rhs != NULL);

	}
	virtual char getOpSymbol() const = 0;

	Expr	*e_lhs;
	Expr	*e_rhs;
private:
	BinArithOp() {}
};

class AOPNOP : public BinArithOp
{
public:
	AOPNOP(Expr* e1, Expr* e2)
		: BinArithOp(e1, e2)
	{
	}
	virtual ~AOPNOP() {}
protected:
	virtual char getOpSymbol() const { return '_'; }
private:
};


class AOPOr : public BinArithOp
{
public:
	AOPOr(Expr* e1, Expr* e2)
		: BinArithOp(e1, e2)
	{
	}
	virtual ~AOPOr() {}
protected:
	virtual char getOpSymbol() const { return '|'; }
private:
};

class AOPAnd : public BinArithOp
{
public:
	AOPAnd(Expr* e1, Expr* e2)
		: BinArithOp(e1, e2)
	{
	}
	virtual ~AOPAnd() {}
protected:
	virtual char getOpSymbol() const { return '&'; }
private:
};



class AOPAdd : public BinArithOp
{
public:
	AOPAdd(Expr* e1, Expr* e2)
		: BinArithOp(e1, e2)
	{
		e_lhs = e1;
	}
	virtual ~AOPAdd() {}
protected:
	virtual char getOpSymbol() const { return '+'; }
private:
};

class AOPSub : public BinArithOp
{
public:
	AOPSub(Expr* e1, Expr* e2) 
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPSub() {} 
protected:
	virtual char getOpSymbol() const { return '-'; }
private:
};

class AOPDiv : public BinArithOp
{
public:
	AOPDiv(Expr* e1, Expr* e2) 
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPDiv() {}
protected:
	virtual char getOpSymbol() const { return '/'; }
private:
};

class AOPMul : public BinArithOp
{
public:
	AOPMul(Expr* e1, Expr* e2)
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPMul() {}
protected:
	virtual char getOpSymbol() const { return '*'; }
private:
};

class AOPLShift : public BinArithOp
{
public:
	AOPLShift(Expr* e1, Expr* e2)
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPLShift() {}
protected:
virtual char getOpSymbol() const { return '<'; }
private:
};

class AOPRShift : public BinArithOp
{
public:
	AOPRShift(Expr* e1, Expr* e2) 
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPRShift() {}
protected:
	virtual char getOpSymbol() const { return '>'; }
private:
};

#endif
