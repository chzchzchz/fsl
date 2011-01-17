#ifndef CONDEXPR_H
#define CONDEXPR_H

#include <list>
#include <iostream>

typedef std::list<const class CondExpr*>	cond_list;

class CondExpr
{
public:
	virtual ~CondExpr() {}

	virtual void expr_rewrite(
		const Expr* to_rewrite, const Expr* replacement) = 0;

	virtual CondExpr* copy(void) const = 0;
	virtual void print(std::ostream& os) const = 0;
protected:
	CondExpr() {}
};

class CondNot : public CondExpr
{
public:
	CondNot(CondExpr* in_cexpr)
	: cexpr(in_cexpr)
	{
		assert (cexpr != NULL);
	}

	virtual ~CondNot()
	{
		delete cexpr;
	}

	const CondExpr* getExpr(void) const { return cexpr; }
	virtual void expr_rewrite(
		const Expr* to_rewrite,
		const Expr* replacement)
	{
		cexpr->expr_rewrite(to_rewrite, replacement);
	}

	virtual CondNot* copy(void) const
	{
		return new CondNot(cexpr->copy());
	}

	virtual void print(std::ostream& os) const {
		os << "!(";
		cexpr->print(os);
		os << ")";
	}
private:
	CondExpr	*cexpr;
	CondNot() {}

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

	void expr_rewrite(const Expr* to_rewrite, const Expr* replacement)
	{
		cond_lhs->expr_rewrite(to_rewrite, replacement);
		cond_rhs->expr_rewrite(to_rewrite, replacement);
	}

	virtual BinBoolOp* copy(void) const = 0;

	virtual void print(std::ostream& os) const = 0;

protected:
	CondExpr	*cond_lhs;
	CondExpr	*cond_rhs;
};

class BOPAnd : public BinBoolOp
{
public:
	BOPAnd(CondExpr* lhs, CondExpr* rhs) : BinBoolOp(lhs, rhs) {}
	virtual ~BOPAnd() {}
	virtual BOPAnd* copy(void) const
	{
		return new BOPAnd(
			cond_lhs->copy(),
			cond_rhs->copy());
	}

	virtual void print(std::ostream& os) const
	{
		os << "(";
		cond_lhs->print(os);
		os << " && ";
		cond_rhs->print(os);
		os << ")";
	}
};

class BOPOr : public BinBoolOp
{
public:
	BOPOr(CondExpr* lhs, CondExpr* rhs) : BinBoolOp(lhs, rhs) {}
	virtual ~BOPOr() {}
	virtual BOPOr* copy(void) const
	{
		return new BOPOr(
			cond_lhs->copy(),
			cond_rhs->copy());
	}

	virtual void print(std::ostream& os) const
	{
		os << "(";
		cond_lhs->print(os);
		os << " || ";
		cond_rhs->print(os);
		os << ")";
	}
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

	void expr_rewrite(const Expr* to_rewrite, const Expr* replacement)
	{
		Expr	*ret;

		ret = lhs->rewrite(to_rewrite, replacement);
		if (ret != NULL) {
			delete lhs;
			lhs = ret;
		}

		ret = rhs->rewrite(to_rewrite, replacement);
		if (ret != NULL) {
			delete rhs;
			rhs = ret;
		}
	}

	virtual CmpOp* copy(void) const = 0;

	virtual void print(std::ostream& os) const
	{
		os << "(";
		lhs->print(os);
		switch (getOp()) {
		case EQ:	os << "=="; break;
		case NE:	os << "!="; break;
		case LE:	os << "<="; break;
		case LT:	os << "<"; break;
		case GT:	os << ">"; break;
		case GE:	os << ">="; break;
		default:	os << "??"; break;
		}
		rhs->print(os);
		os << ")";
	}

protected:
	Expr	*lhs;
	Expr	*rhs;
private:
	CmpOp() {}
};

class CmpEQ : public CmpOp
{
public:
	CmpEQ(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {}
	virtual ~CmpEQ() {}

	Op getOp(void) const { return EQ; }
	virtual CmpEQ* copy(void) const
	{
		return new CmpEQ(lhs->copy(), rhs->copy());
	}
};

class CmpNE : public CmpOp
{
public:
	CmpNE(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {}
	virtual ~CmpNE() {}
	Op getOp(void) const { return NE; }
	virtual CmpNE* copy(void) const
	{
		return new CmpNE(lhs->copy(), rhs->copy());
	}
};

class CmpLE : public CmpOp
{
public:
	CmpLE(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {}
	virtual ~CmpLE() {}

	Op getOp(void) const { return LE; }
	virtual CmpLE* copy(void) const
	{
		return new CmpLE(lhs->copy(), rhs->copy());
	}
};

class CmpGE : public CmpOp
{
public:
	CmpGE(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {}
	virtual ~CmpGE() {}

	Op getOp(void) const { return GE; }

	virtual CmpGE* copy(void) const
	{
		return new CmpGE(lhs->copy(), rhs->copy());
	}
};

class CmpLT : public CmpOp
{
public:
	CmpLT(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {}
	virtual ~CmpLT() {}

	Op getOp(void) const { return LT; }

	virtual CmpLT* copy(void) const
	{
		return new CmpLT(lhs->copy(), rhs->copy());
	}
};

class CmpGT : public CmpOp
{
public:
	CmpGT(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {}
	virtual ~CmpGT() {}
	Op getOp(void) const { return GT; }

	virtual CmpGT* copy(void) const
	{
		return new CmpGT(lhs->copy(), rhs->copy());
	}
};

class FuncCond : public CondExpr
{
public:
	FuncCond(FCall* in_fc) : fc(in_fc) { assert (fc != NULL); }
	virtual ~FuncCond() { delete fc; }
	void expr_rewrite(const Expr*, const Expr*) { assert (0 == 1); }
	const FCall* getFC(void) const { return fc; }

	virtual FuncCond* copy(void) const
	{
		return new FuncCond(fc->copy());
	}

	virtual void print(std::ostream& os) const
	{
		fc->print(os);
	}
private:
	FCall*	fc;
};

#endif
