#ifndef BINARITHOP_H
#define BINARITHOP_H

#include <iostream>

#define ulong	unsigned long

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

	void simplifySides(void)
	{
		Expr	*new_lhs, *new_rhs;

		if (simplified == true)
			return;

		new_lhs = e_lhs->simplify();
		new_rhs = e_rhs->simplify();
		if (new_lhs != e_lhs) delete e_lhs;
		if (new_rhs != e_rhs) delete e_rhs;
		e_lhs = new_lhs;
		e_rhs = new_rhs;
		simplified = true;
	}

	bool toNumbers(unsigned long& n_lhs, unsigned long& n_rhs) const
	{
		Number	*lhs, *rhs;

		const_cast<BinArithOp*>(this)->simplifySides();

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

	bool canSimplify(void) const
	{
		return (dynamic_cast<Number*>(e_lhs) != NULL &&
			dynamic_cast<Number*>(e_rhs) != NULL);
	}

	void setLHS(Expr* e)
	{
		assert (e != NULL);
		if (e == e_lhs) return;

		delete e_lhs;

		e_lhs = e->simplify();
		delete e;
	}

	void setRHS(Expr* e)
	{
		assert (e != NULL);
		if (e == e_rhs) return;

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

	virtual Expr* rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		Expr*	ret;

		ret = e_lhs->rewrite(to_rewrite, new_expr);
		if (ret != NULL) {
			delete e_lhs;
			e_lhs = ret;
		}

		ret = e_rhs->rewrite(to_rewrite, new_expr);
		if (ret != NULL) {
			delete e_rhs;
			e_rhs = ret;
		}

		return Expr::rewrite(to_rewrite, new_expr);
	}

	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }

	Expr* simplify(void) const
	{
		unsigned long	n_lhs, n_rhs;

		if (toNumbers(n_lhs, n_rhs) == false)
			return copy();

		return new Number(doOp(n_lhs, n_rhs));
	}

protected:
	BinArithOp(Expr* e1, Expr* e2)
		: e_lhs(e1), e_rhs(e2), simplified(false)
	{
		assert (e_lhs != NULL);
		assert(e_rhs != NULL);
	}
	virtual char getOpSymbol() const = 0;
	virtual ulong doOp(ulong lhs, ulong rhs) const = 0;

	Expr	*e_lhs;
	Expr	*e_rhs;
private:
	BinArithOp() {}
	bool	simplified;
};

class AOPOr : public BinArithOp
{
public:
	AOPOr(Expr* e1, Expr* e2) : BinArithOp(e1, e2) { }
	virtual ~AOPOr() {}

	Expr* copy(void) const
	{
		return new AOPOr(e_lhs->copy(), e_rhs->copy());
	}

	llvm::Value* codeGen() const;

protected:
	ulong doOp(ulong lhs, ulong rhs) const { return (lhs | rhs); }
	virtual char getOpSymbol() const { return '|'; }

private:
};

class AOPAnd : public BinArithOp
{
public:
	AOPAnd(Expr* e1, Expr* e2) : BinArithOp(e1, e2) { }
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

	llvm::Value* codeGen() const;

protected:
	ulong doOp(ulong lhs, ulong rhs) const { return (lhs & rhs); }
	virtual char getOpSymbol() const { return '&'; }
private:
};



class AOPAdd : public BinArithOp
{
public:
	AOPAdd(Expr* e1, Expr* e2) : BinArithOp(e1, e2) {}

	virtual ~AOPAdd() {}

	Expr* copy(void) const
	{
		return new AOPAdd(e_lhs->copy(), e_rhs->copy());
	}


	llvm::Value* codeGen() const;

protected:
	ulong doOp(ulong lhs, ulong rhs) const { return (lhs + rhs); }
	virtual char getOpSymbol() const { return '+'; }
private:
};

class AOPSub : public BinArithOp
{
public:
	AOPSub(Expr* e1, Expr* e2) : BinArithOp(e1, e2) {}

	virtual ~AOPSub() {}

	Expr* copy(void) const
	{
		return new AOPSub(e_lhs->copy(), e_rhs->copy());
	}

	llvm::Value* codeGen() const;

protected:
	ulong doOp(ulong lhs, ulong rhs) const { return (lhs - rhs); }
	virtual char getOpSymbol() const { return '-'; }
private:
};

class AOPDiv : public BinArithOp
{
public:
	AOPDiv(Expr* e1, Expr* e2) : BinArithOp(e1, e2) { }

	virtual ~AOPDiv() {}

	Expr* copy(void) const
	{
		return new AOPDiv(e_lhs->copy(), e_rhs->copy());
	}

	llvm::Value* codeGen() const;

protected:
	ulong doOp(ulong lhs, ulong rhs) const {
		assert (rhs != 0);
		return (lhs / rhs);
	}
	virtual char getOpSymbol() const { return '/'; }
private:
};

class AOPMul : public BinArithOp
{
public:
	AOPMul(Expr* e1, Expr* e2) : BinArithOp(e1, e2) { }

	virtual ~AOPMul() {}

	Expr* copy(void) const
	{
		return new AOPMul(e_lhs->copy(), e_rhs->copy());
	}

	llvm::Value* codeGen() const;
protected:
	ulong doOp(ulong lhs, ulong rhs) const { return (lhs*rhs); }
	virtual char getOpSymbol() const { return '*'; }
private:
};

class AOPLShift : public BinArithOp
{
public:
	AOPLShift(Expr* e1, Expr* e2) : BinArithOp(e1, e2) { }

	virtual ~AOPLShift() {}

	Expr* copy(void) const
	{
		return new AOPLShift(e_lhs->copy(), e_rhs->copy());
	}

	llvm::Value* codeGen() const;
protected:
	ulong doOp(ulong lhs, ulong rhs) const { return (lhs<<rhs); }
virtual char getOpSymbol() const { return '<'; }
private:
};

class AOPRShift : public BinArithOp
{
public:
	AOPRShift(Expr* e1, Expr* e2) : BinArithOp(e1, e2) { }

	virtual ~AOPRShift() {}

	Expr* copy(void) const
	{
		return new AOPRShift(e_lhs->copy(), e_rhs->copy());
	}

	llvm::Value* codeGen() const;
protected:
	ulong doOp(ulong lhs, ulong rhs) const { return (lhs>>rhs); }
	virtual char getOpSymbol() const { return '>'; }
private:
};

class AOPMod : public BinArithOp
{
public:
	AOPMod(Expr* e1, Expr* e2) : BinArithOp(e1, e2) { }

	virtual ~AOPMod() {}

	Expr* copy(void) const
	{
		return new AOPMod(e_lhs->copy(), e_rhs->copy());
	}

	llvm::Value* codeGen() const;
protected:
	ulong doOp(ulong lhs, ulong rhs) const { return (lhs%rhs); }
	virtual char getOpSymbol() const { return '%'; }
};


#endif
