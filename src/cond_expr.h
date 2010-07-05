#ifndef CONDEXPR_H
#define CONDEXPR_H

class CondExpr 
{
public:
	virtual ~CondExpr() {}

	virtual void expr_rewrite(
		const Expr* to_rewrite, const Expr* replacement) = 0;
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

	void expr_rewrite(const Expr* to_rewrite, const Expr* replacement)
	{
		cond_lhs->expr_rewrite(to_rewrite, replacement);
		cond_rhs->expr_rewrite(to_rewrite, replacement);
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

class FuncCond : public CondExpr 
{
public: 
	FuncCond(FCall* in_fc) : fc(in_fc) { assert (fc != NULL); } 
	virtual ~FuncCond() { delete fc; } 
	void expr_rewrite(const Expr*, const Expr*) { assert (0 == 1); }
	const FCall* getFC(void) const { return fc; }
private:
	FCall*	fc;
};

#endif
