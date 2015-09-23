#ifndef CONDEXPR_H
#define CONDEXPR_H

#include <list>
#include <iostream>

typedef std::list<const class CondExpr*>	cond_list;

class EvalCtx;

class CondExpr
{
public:
	virtual ~CondExpr() {}
	virtual void expr_rewrite(const Expr* replacee, const Expr* replacer)=0;
	virtual CondExpr* copy(void) const = 0;
	virtual void print(std::ostream& os) const = 0;
	/** generates boolean value for given condition */
	virtual llvm::Value* codeGen(const EvalCtx* ctx = NULL) const = 0;
protected:
	CondExpr() {}
};

class CondNot : public CondExpr
{
public:
	CondNot(CondExpr* in_cexpr)
	: cexpr(in_cexpr) { assert (cexpr != NULL); }

	virtual ~CondNot() { delete cexpr; }

	const CondExpr* getExpr(void) const { return cexpr; }
	void expr_rewrite(const Expr* to_rewrite, const Expr* replacer)
	{
		cexpr->expr_rewrite(to_rewrite, replacer);
	}

	virtual CondNot* copy(void) const { return new CondNot(cexpr->copy()); }

	virtual void print(std::ostream& os) const {
		os << "(not "; cexpr->print(os); os <<  ")";
	}
	llvm::Value* codeGen(const EvalCtx* ctx) const;
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
	llvm::Value* codeGen(const EvalCtx* ctx) const = 0;
protected:
	CondExpr	*cond_lhs, *cond_rhs;
};

#define BOP_COPY(x)	virtual x* copy() const				\
	{ return new x(cond_lhs->copy(), cond_rhs->copy()); }
#define BOP_PRINT(x) virtual void print(std::ostream& os) const	\
	{ os << "("; cond_lhs->print(os); os << x; cond_rhs->print(os); \
	os << ")"; }

class BOPAnd : public BinBoolOp
{
public:
	BOPAnd(CondExpr* lhs, CondExpr* rhs) : BinBoolOp(lhs, rhs) {}
	virtual ~BOPAnd() {}
	BOP_COPY(BOPAnd)
	BOP_PRINT(" && ")
	llvm::Value* codeGen(const EvalCtx* ctx) const;
};

class BOPOr : public BinBoolOp
{
public:
	BOPOr(CondExpr* lhs, CondExpr* rhs) : BinBoolOp(lhs, rhs) {}
	virtual ~BOPOr() {}
	BOP_COPY(BOPOr);
	BOP_PRINT(" || ");
	llvm::Value* codeGen(const EvalCtx* ctx) const;
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

	llvm::Value* codeGen(const EvalCtx* ctx) const;
protected:
	Expr	*lhs, *rhs;
private:
	CmpOp() {}
};

#define CMP_COPY(x) virtual x* copy(void) const 	\
{ 							\
	return new x(lhs->copy(), rhs->copy()); 	\
}
#define CMP_GETOP(x) Op getOp(void) const { return x; }
#define CMP_CLASS(x,y) class x : public CmpOp		\
{							\
public:							\
	x(Expr* lhs, Expr* rhs) : CmpOp(lhs, rhs) {}	\
	virtual ~x() {}					\
	CMP_GETOP(y)					\
	CMP_COPY(x)					\
}

CMP_CLASS(CmpEQ, EQ);
CMP_CLASS(CmpNE, NE);
CMP_CLASS(CmpLE, LE);
CMP_CLASS(CmpGE, GE);
CMP_CLASS(CmpLT, LT);
CMP_CLASS(CmpGT, GT);

class FuncCond : public CondExpr
{
public:
	FuncCond(FCall* in_fc) : fc(in_fc) { assert (fc != NULL); }
	virtual ~FuncCond() { delete fc; }
	void expr_rewrite(const Expr*, const Expr*) { assert (0 == 1); }
	const FCall* getFC(void) const { return fc; }

	virtual FuncCond* copy(void) const { return new FuncCond(fc->copy()); }
	virtual void print(std::ostream& os) const { fc->print(os); }
	llvm::Value* codeGen(const EvalCtx* ctx) const;
private:
	FCall*	fc;
};

#endif
