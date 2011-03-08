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

	std::ostream& print(std::ostream& out) const
	{
		return out << '(' <<
			e_lhs->print(out) << ' ' << getOpSymbol() << ' ' <<
			e_rhs->print(out) << ')';
	}

	void simplifySides(void)
	{
		Expr	*new_lhs, *new_rhs;

		if (simplified == true) return;

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
		if (lhs == NULL || rhs == NULL) return false;

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
		if (in_bop == NULL) return false;
		if (in_bop->getOpSymbol() != getOpSymbol()) return false;
		if (*e_lhs != in_bop->getLHS()) return false;
		if (*e_rhs != in_bop->getRHS()) return false;

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
		if (toNumbers(n_lhs, n_rhs) == false) return copy();
		return new Number(doOp(n_lhs, n_rhs));
	}

protected:
	BinArithOp(Expr* e1, Expr* e2)
		: e_lhs(e1), e_rhs(e2), simplified(false)
	{
		assert (e_lhs != NULL);
		assert(e_rhs != NULL);
	}
	virtual const char* getOpSymbol() const = 0;
	virtual ulong doOp(ulong lhs, ulong rhs) const = 0;
	Expr	*e_lhs, *e_rhs;
private:
	BinArithOp() {}
	bool	simplified;
};

#define AOP_COPY(x)	\
Expr* copy(void) const { return new x(e_lhs->copy(), e_rhs->copy()); }
#define AOP_CLASS(x,y)						\
class x : public BinArithOp					\
{								\
public:								\
	x(Expr* e1, Expr* e2) : BinArithOp(e1, e2) { }		\
	virtual ~x() {}						\
	AOP_COPY(x)						\
	llvm::Value* codeGen() const;				\
protected:							\
	ulong doOp(ulong lhs, ulong rhs) const { return (lhs y rhs); }	\
	virtual const char* getOpSymbol() const { return #y; }	\
private:							\
}

AOP_CLASS(AOPAnd, &);
AOP_CLASS(AOPOr, |);
AOP_CLASS(AOPAdd, +);
AOP_CLASS(AOPSub, -);
AOP_CLASS(AOPDiv, /);
AOP_CLASS(AOPMul, *);
AOP_CLASS(AOPLShift, <<);
AOP_CLASS(AOPRShift, >>);
AOP_CLASS(AOPMod, %);

#endif
