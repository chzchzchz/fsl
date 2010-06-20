#ifndef EXPR_H
#define EXPR_H

#include "collection.h"

class Expr
{
public:
	virtual void print(std::ostream& out) const = 0;
	virtual ~Expr() {}
	virtual Expr* copy(void) const = 0;
protected:
	Expr() {}
};

class ExprList : public PtrList<Expr>
{
public:
	ExprList() {}
	ExprList(const PtrList<Expr>& p) : PtrList<Expr>(p) {}
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
	ExprList* copy(void) const
	{
		return new ExprList(*this);
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
	Id* copy(void) const { return new Id(id_name); }
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
	const std::string& getName(void) const { return id->getName(); }
	void print(std::ostream& out) const
	{
		out << "fcall " << getName() << "("; 
		exprs->print(out);
		out << ")";
	}

	FCall* copy(void) const 
	{
		return new FCall(id->copy(), exprs->copy());
	}

private:
	Id*		id;
	ExprList*	exprs;
};

class IdStruct : public Expr, public PtrList<Expr>
{
public:
	IdStruct() { }
	IdStruct(const PtrList<Expr>& p) : PtrList<Expr>(p) {}
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

	IdStruct* copy(void) const 
	{
		return new IdStruct(*this);
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

	Id* getId(void) const { return id; }
	Expr* getIdx(void) const { return idx; }

	IdArray* copy(void) const
	{
		return new IdArray(id->copy(), idx->copy());
	}

private:
	Id		*id;
	Expr		*idx;
};


class ExprParens : public Expr
{
public:
	ExprParens(Expr* in_expr)
		: expr(in_expr)
	{
		assert (expr != NULL);
	}


	virtual ~ExprParens() { delete expr; }

	void print(std::ostream& out) const
	{
		out << '(';
		expr->print(out);
		out << ')';
	}

	Expr* copy(void) const
	{
		return new ExprParens(expr->copy());
	}

private:
	Expr	*expr;
};

class Number : public Expr
{
public:
	Number(unsigned long v) : n(v) {}
	virtual ~Number() {}

	unsigned long getValue() const { return n; }

	void print(std::ostream& out) const { out << n; }

	Expr* copy(void) const { return new Number(n); }
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
		out << '(';
		e_lhs->print(out);
		out << ' ' << getOpSymbol() << ' ';
		e_rhs->print(out);
		out << ')';
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
		: BinArithOp(e1, e2) {}
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

	Expr* copy(void) const
	{
		return new AOPOr(e_lhs->copy(), e_rhs->copy()); 
	}

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

	Expr* copy(void) const
	{
		return new AOPAnd(e_lhs->copy(), e_rhs->copy()); 
	}

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

	Expr* copy(void) const
	{
		return new AOPAdd(e_lhs->copy(), e_rhs->copy()); 
	}

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

	Expr* copy(void) const
	{
		return new AOPSub(e_lhs->copy(), e_rhs->copy()); 
	}


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

	Expr* copy(void) const
	{
		return new AOPDiv(e_lhs->copy(), e_rhs->copy()); 
	}

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

	Expr* copy(void) const
	{
		return new AOPMul(e_lhs->copy(), e_rhs->copy()); 
	}
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

	Expr* copy(void) const
	{
		return new AOPLShift(e_lhs->copy(), e_rhs->copy());
	}

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

	Expr* copy(void) const
	{
		return new AOPRShift(e_lhs->copy(), e_rhs->copy()); 
	}

protected:
	virtual char getOpSymbol() const { return '>'; }
private:
};

class AOPMod : public BinArithOp
{
public:
	AOPMod(Expr* e1, Expr* e2) 
		: BinArithOp(e1, e2)
	{
	}

	virtual ~AOPMod() {}

	Expr* copy(void) const
	{
		return new AOPMod(e_lhs->copy(), e_rhs->copy()); 
	}

protected:
	virtual char getOpSymbol() const { return '%'; }
};

#endif
