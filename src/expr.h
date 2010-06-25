#ifndef EXPR_H
#define EXPR_H

#include <map>
#include "collection.h"

typedef std::map<std::string, class Expr*>	const_map;

class Expr
{
public:
	virtual void print(std::ostream& out) const = 0;
	virtual ~Expr() {}
	virtual Expr* copy(void) const = 0;
	virtual Expr* simplify(void) const { return this->copy(); }
	virtual bool operator==(const Expr* e) const = 0;
	bool operator!=(const Expr* e) const
	{
		return ((*this == e) == false);
	}
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

	ExprList* simplify(void) const
	{
		ExprList*	ret;

		ret = new ExprList();
		for (const_iterator it = begin(); it != end(); it++) {
			ret->add((*it)->simplify());
		}

		return ret;
	}

	bool operator==(const ExprList* in_list) const
	{
		const_iterator	it, it2;

		if (in_list == this) return true;

		if (in_list->size() != size()) return false;

		for (	it = begin(), it2 = in_list->begin();
			it != end();
			it++, it2++)
		{
			const Expr	*us(*it), *them(*it2);

			if (*us != them)
				return false;
		}

		return true;
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

	bool operator==(const Expr* e) const
	{
		const Id*	in_id = dynamic_cast<const Id*>(e);
		if (in_id == NULL) return false;
		return (id_name == in_id->getName());
	}
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
		exprs = in_exprs->simplify();
		delete in_exprs;
	}
	virtual ~FCall()
	{
		delete id;
		delete exprs;
	}

	const ExprList* getExprs(void) const { return exprs; }
	ExprList* getExprs(void) { return exprs; }
	void setExprs(ExprList* new_exprs) 
	{
		assert (new_exprs != NULL);
		assert (new_exprs->size() == exprs->size());

		delete exprs;
		exprs = new_exprs->simplify();
		delete new_exprs;
	}

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

	bool operator==(const Expr* e) const
	{
		const FCall	*in_fc;

		in_fc = dynamic_cast<const FCall*>(e);
		if (in_fc == NULL) return false;

		return (in_fc->id == id) && (*exprs == in_fc->exprs);
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

	bool operator==(const Expr* e) const
	{
		const IdStruct	*in_struct;
		const_iterator	it, it2;

		in_struct = dynamic_cast<const IdStruct*>(e);
		if (in_struct == NULL)
			return false;

		if (size() != in_struct->size())
			return false;

		for (	it = begin(), it2 = in_struct->begin(); 
			it != end();
			++it, ++it2)
		{
			
			Expr	*our_expr = *it;
			Expr	*in_expr = *it2;

			if (*our_expr != in_expr)
				return false;
		}

		return true;
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

	bool operator==(const Expr* e) const
	{
		const IdArray	*in_array;

		in_array = dynamic_cast<const IdArray*>(e);
		if (in_array == NULL)
			return false;

		return ((*id == in_array->id) && (*idx == in_array->idx));
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

	Expr* simplify(void) const
	{
		return new ExprParens(expr->simplify());
	}

	Expr* getExpr(void) { return expr; }
	const Expr* getExpr(void) const { return expr; }
	void setExpr(Expr* e) 
	{
		assert (e != NULL);
		delete expr;
		expr = e;
	}

	bool operator==(const Expr* e) const 
	{
		const ExprParens	*in_parens;

		in_parens = dynamic_cast<const ExprParens*>(e);
		if (in_parens == NULL)
			return false;
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

	bool operator==(const Expr* e) const
	{
		const Number	*in_num;

		in_num = dynamic_cast<const Number*>(e);
		if (in_num == NULL)
			return false;

		return (n == in_num->getValue());
	}
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

	bool toNumbers(unsigned long& n_lhs, unsigned long& n_rhs) const
	{
		Number	*lhs, *rhs;

		lhs = dynamic_cast<Number*>(e_lhs);
		rhs = dynamic_cast<Number*>(e_rhs);
		if (lhs == NULL || rhs == NULL)
			return false;

		n_lhs = lhs->getValue();
		n_rhs = rhs->getValue();

		return true;
	}

	Expr* getLHS() { return e_lhs; }
	Expr* getRHS() { return e_rhs; }
	const Expr* getLHS() const { return e_lhs; }
	const Expr* getRHS() const { return e_rhs; }

	void setLHS(Expr* e) 
	{
		assert (e != NULL);
		delete e_lhs;

		e_lhs = e->simplify();
		delete e;
	}

	void setRHS(Expr* e) 
	{
		assert (e != NULL);
		delete e_rhs;

		e_rhs = e->simplify();
		delete e;
	}

	bool operator==(const Expr* e) const
	{
		const BinArithOp *in_bop;
		
		in_bop = dynamic_cast<const BinArithOp*>(e);
		if (in_bop == NULL)
			return false;

		if (in_bop->getOpSymbol() != getOpSymbol())
			return false;

		if (*e_lhs != in_bop->getLHS())
			return false;

		if (*e_rhs != in_bop->getRHS())
			return false;

		return true;
	}

protected:
	BinArithOp(Expr* e1, Expr* e2)
		: e_lhs(e1), e_rhs(e2) 
	{
		Expr	*new_lhs, *new_rhs;

		assert (e_lhs != NULL);
		assert(e_rhs != NULL);

		new_lhs = e_lhs->simplify();
		new_rhs = e_rhs->simplify();

		delete e_lhs;
		delete e_rhs;

		e_lhs = new_lhs;
		e_rhs = new_rhs;
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

	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		return new Number(n_lhs | n_rhs);
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

	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		return new Number(n_lhs & n_rhs);
	}

protected:
	virtual char getOpSymbol() const { return '&'; }
private:
};



class AOPAdd : public BinArithOp
{
public:
	AOPAdd(Expr* e1, Expr* e2)
		: BinArithOp(e1, e2) {}

	virtual ~AOPAdd() {}

	Expr* copy(void) const
	{
		return new AOPAdd(e_lhs->copy(), e_rhs->copy()); 
	}

	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		return new Number(n_lhs + n_rhs);
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


	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		return new Number(n_lhs - n_rhs);
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

	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		if (n_rhs == 0) {
			std::cerr << "Dividing by zero in ";
			print(std::cerr);
			std::cerr << std::endl;
			return NULL;
		}

		return new Number(n_lhs / n_rhs);
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

	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		return new Number(n_lhs * n_rhs);
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

	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		return new Number(n_lhs << n_rhs);
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

	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		return new Number(n_lhs >> n_rhs);
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

	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		return new Number(n_lhs % n_rhs);
	}

protected:
	virtual char getOpSymbol() const { return '%'; }
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
	BinBoolOp(CondExpr* in_lhs, CondExpr* in_rhs) 
	: cond_lhs(in_lhs), cond_rhs(in_rhs)
	{
		assert (cond_lhs != NULL);
		assert (cond_rhs != NULL);
	}

	const CondExpr* getLHS() const { return cond_lhs; }
	const CondExpr* getRHS() const { return cond_rhs; } 

	virtual ~BinBoolOp() 
	{
		delete cond_lhs;
		delete cond_rhs;
	}
private:
	CondExpr	*cond_lhs;
	CondExpr	*cond_rhs;
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
	CmpOp(Expr* in_lhs, Expr* in_rhs) 
	: lhs(in_lhs), rhs(in_rhs)
	{
		assert (lhs != NULL);
		assert (rhs != NULL);
	}
	virtual ~CmpOp() 
	{
		delete lhs;
		delete rhs;
	} 

	enum Op {EQ, NE, LE, LT, GT, GE};
	virtual Op getOp(void) const = 0;

	const Expr* getLHS() const { return lhs; }
	const Expr* getRHS() const { return rhs; }

private:
	Expr	*lhs;
	Expr	*rhs;

	CmpOp() {}
};

class CmpEQ : public CmpOp 
{
public:
	CmpEQ(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpEQ() {}

	Op getOp(void) const { return EQ; }
};

class CmpNE : public CmpOp 
{
public:
	CmpNE(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpNE() {} 
	Op getOp(void) const { return NE; }
};

class CmpLE : public CmpOp
{
public:
	CmpLE(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpLE() {} 

	Op getOp(void) const { return LE; }
};

class CmpGE : public CmpOp
{
public:
	CmpGE(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpGE() {} 

	Op getOp(void) const { return GE; }
};

class CmpLT : public CmpOp
{
public:
	CmpLT(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpLT() {} 

	Op getOp(void) const { return LT; }
};

class CmpGT : public CmpOp
{
public:
	CmpGT(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {} 
	virtual ~CmpGT() {} 
	Op getOp(void) const { return GT; }
};


#endif
