#ifndef EXPR_H
#define EXPR_H

#include <iostream>
#include <map>
#include <llvm/IR/DerivedTypes.h>
#include "collection.h"

typedef std::map<std::string, class Expr*>		const_map;
typedef std::map<const class Expr, const class Expr*>	rewrite_map;

class Id;
class IdArray;
class FCall;
class BinArithOp;
class IdStruct;
class ExprParens;
class CondVal;

class ExprVisitor
{
public:
	virtual Expr* apply(const Expr* e) = 0;
	virtual Expr* applyReplace(Expr* &e) = 0;

	virtual Expr* visit(const Expr* a) = 0;
	virtual Expr* visit(const Id* a)  = 0;
	virtual Expr* visit(const IdArray* a) = 0;
	virtual Expr* visit(const FCall* a) = 0;
	virtual Expr* visit(const BinArithOp* a) = 0;
	virtual Expr* visit(const IdStruct* a) = 0;
	virtual Expr* visit(const ExprParens* a) = 0;
	virtual Expr* visit(const CondVal* a) = 0;

	virtual ~ExprVisitor() {}
protected:
	ExprVisitor() {}
};

class Expr
{
public:
	virtual void print(std::ostream& out) const = 0;
	virtual ~Expr() {}
	virtual Expr* copy(void) const = 0;
	virtual Expr* simplify(void) const { return this->copy(); }
	virtual bool operator==(const Expr* e) const = 0;
	bool operator!=(const Expr* e) const { return ((*this == e) == false); }

	virtual Expr* rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		assert (to_rewrite && "no target expr");
		assert (new_expr && "no replacement expr");
		assert (this);
		if (*this == to_rewrite) return new_expr->simplify();
		return NULL;
	}

	/**
	 *  deletes last two expressions
	 *  returns what 'e' should be set to
	 *
	 *  e = rewriteReplace(e, to_rewrite, new_expr);
	 */
	static Expr* rewriteReplace(
		Expr* e,
		Expr* to_rewrite,
		Expr* new_expr)
	{
		Expr	*rewritten_expr;

		assert (e != NULL && "Bad expression to rewrite");
		assert (to_rewrite != NULL && "Bad removal expr");
		assert (new_expr != NULL && "Bad replacement expr");

		rewritten_expr = e->rewrite(to_rewrite, new_expr);
		delete to_rewrite;
		delete new_expr;
		if (rewritten_expr == NULL) return e;

		delete e;
		return rewritten_expr;;
	}

	llvm::Value* ErrorV(const std::string& s) const;

	virtual llvm::Value* codeGen(void) const = 0;
	virtual Expr* accept(ExprVisitor* ev) const = 0;
};

class ExprList : public PtrList<Expr>
{
public:
	ExprList() {}
	ExprList(Expr* e) { add(e); }

	ExprList(Expr* e, Expr* e2) {
		assert (e != NULL && e2 != NULL);
		add(e); add(e2);
	}

	ExprList(const PtrList<Expr>& p) : PtrList<Expr>(p) {}
	virtual ~ExprList()  {}

	void print(std::ostream& out) const {
		const_iterator	it = begin();

		if (it == end()) return;

		(*it)->print(out); it++;
		for (; it != end(); it++) {
			out << ", ";
			(*it)->print(out);
		}
	}

	ExprList* copy(void) const { return new ExprList(*this); }

	ExprList* simplify(void) const {
		auto ret = new ExprList();
		for (auto &e : *this) ret->add(e->simplify());
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
			const Expr	*us(it->get()), *them(it2->get());
			if (*us != them) return false;
		}

		return true;
	}

	void rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		assert (to_rewrite);
		assert (new_expr);

		for (auto & e : *this) {
			Expr	*tmp_expr;

			assert (e != nullptr);

			tmp_expr = e->rewrite(to_rewrite, new_expr);
			if (tmp_expr == NULL) continue;

			e.reset(tmp_expr);
		}
	}

	const Expr* getNth(unsigned int n) const {
		const_iterator it = begin();
		for (unsigned int i = 0; i < n; it++,i++);
		return it->get();
	}
private:
};

class Id  : public Expr
{
public:
	Id(const std::string& s) : id_name(s) { assert (s.size() > 0); }
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

	llvm::Value* codeGen(void) const;
	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
private:
	const std::string id_name;
};

#include "fcall.h"

class IdStruct : public Expr, public PtrList<Expr>
{
public:
	IdStruct() { }
	IdStruct(const PtrList<Expr>& p) : PtrList<Expr>(p) {}
	virtual ~IdStruct() {}
	void print(std::ostream& out) const
	{
		const_iterator	it = begin();

		assert (it != end());

		(*it)->print(out);
		it++;
		for (; it != end(); it++) {
			out << '.';
			(*it)->print(out);
		}
	}

	IdStruct* copy(void) const { return new IdStruct(*this); }

	bool operator==(const Expr* e) const
	{
		const IdStruct	*in_struct;
		const_iterator	it, it2;

		in_struct = dynamic_cast<const IdStruct*>(e);
		if (in_struct == NULL) return false;
		if (size() != in_struct->size()) return false;

		for (	it = begin(), it2 = in_struct->begin();
			it != end();
			++it, ++it2)
		{
			const Expr	*our_expr = it->get();
			const Expr	*in_expr = it2->get();
			if (*our_expr != in_expr) return false;
		}

		return true;
	}

	llvm::Value* codeGen(void) const;
	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
private:
};


class IdArray  : public Expr
{
public:
	IdArray(Id *in_id, Expr* expr) :
		id(in_id), idx(expr), fixed(false), nofollow(false)
	{
		assert (id != NULL);
		assert (idx != NULL);
	}

	virtual ~IdArray()
	{
		delete id;
		delete idx;
	}

	const std::string& getName() const { return id->getName(); }

	void print(std::ostream& out) const
	{
		id->print(out);
		out << '[';
		idx->print(out);
		out << ']';
	}

	Id* getId(void) const { return id; }
	const Expr* getIdx(void) const { return idx; }
	Expr* getIdx(void) { return idx; }
	void setIdx(Expr* in_idx)
	{
		assert (in_idx != NULL);
		assert (idx != NULL);
		if (in_idx == idx) return;
		delete idx;
		idx = in_idx;
	}

	IdArray* copy(void) const
	{
		IdArray	*ret = new IdArray(id->copy(), idx->copy());
		ret->fixed = fixed;
		ret->nofollow = nofollow;
		return ret;
	}

	bool operator==(const Expr* e) const
	{
		const IdArray	*in_array;

		in_array = dynamic_cast<const IdArray*>(e);
		if (in_array == NULL) return false;

		return ((*id == in_array->id) && (*idx == in_array->idx));
	}

	virtual Expr* rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		Expr*	ret;

		ret = idx->rewrite(to_rewrite, new_expr);
		if (ret != NULL) {
			delete idx;
			idx = ret;
		}

		return Expr::rewrite(to_rewrite, new_expr);
	}

	llvm::Value* codeGen(void) const;
	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }

	bool isFixed(void) const { return fixed; }
	bool isNoFollow(void) const { return nofollow; }
	void setFixed(bool f) { fixed = f; }
	void setNoFollow(bool f) { nofollow = f; }
private:
	Id		*id;
	Expr		*idx;
	bool		fixed;
	bool		nofollow;
};

class ExprParens : public Expr
{
public:
	ExprParens(Expr* in_expr)
		: expr(in_expr) { assert (expr != NULL); }


	virtual ~ExprParens() { delete expr; }

	void print(std::ostream& out) const
	{
		out << '('; expr->print(out); out << ')';
	}

	Expr* copy(void) const { return new ExprParens(expr->copy()); }
	Expr* simplify(void) const { return new ExprParens(expr->simplify()); }

	Expr* getExpr(void) { return expr; }
	const Expr* getExpr(void) const { return expr; }
	void setExpr(Expr* e)
	{
		assert (e != NULL);
		if (e == expr) return;
		delete expr;
		expr = e;
	}

	bool operator==(const Expr* e) const
	{
		const ExprParens	*in_parens;

		in_parens = dynamic_cast<const ExprParens*>(e);
		if (in_parens == NULL) return false;

		return (*expr == (in_parens->getExpr()));
	}

	virtual Expr* rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		Expr*	ret;

		ret = expr->rewrite(to_rewrite, new_expr);
		if (ret != NULL) {
			delete expr;
			expr = ret;
		}

		return NULL;
	}

	llvm::Value* codeGen(void) const;
	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
private:
	Expr	*expr;
};

class Boolean : public Expr
{
public:
	Boolean(bool v) : b(v) {}
	virtual ~Boolean() {}

	void print(std::ostream& out) const { out << b; }

	Expr* copy(void) const { return new Boolean(b); }

	bool operator==(const Expr* e) const
	{
		const Boolean	*in_num = dynamic_cast<const Boolean*>(e);
		if (in_num == NULL) return false;
		return (b == in_num->isTrue());
	}

	bool isTrue(void) const { return b; }

	llvm::Value* codeGen(void) const;
	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
private:
	unsigned long b;

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
		const Number	*in_num = dynamic_cast<const Number*>(e);
		if (in_num == NULL) return false;
		return (n == in_num->getValue());
	}

	llvm::Value* codeGen(void) const;
	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
private:
	unsigned long n;
};

#include "binarithop.h"
#include "cond.h"

/* not exposed to user.. placeholder for (cond_expr) ? a : b.. */
class CondVal : public Expr
{
public:
	CondVal(CondExpr* in_ce, Expr* in_e_t, Expr* in_e_f)
	: ce(in_ce), e_t(in_e_t), e_f(in_e_f) {}

	virtual ~CondVal();

	void print(std::ostream& os) const
	{
		os	<< "(condval "; ce->print(os); os << "\n ";
			e_t->print(os); os << "\n "; e_f->print(os); os << ")";
	}

	CondVal* copy(void) const;

	bool operator==(const Expr* e) const
	{
		const CondVal*	cv = dynamic_cast<const CondVal*>(e);
		if (cv == NULL) return false;
		/* XXX need *ce == cv->ce, but condexpr doesn't have == !!!) */
		return (*e_t == cv->e_t && *e_f == cv->e_f);
	}

	llvm::Value* codeGen(void) const;
	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
	const Expr* getTrue(void) const { return e_t; }
	const Expr* getFalse(void) const { return e_f; }
	const CondExpr* getCond(void) const { return ce; }
private:
	CondExpr	*ce;
	Expr		*e_t, *e_f;
};

class CondOrExpr {
public:
	CondOrExpr(Expr* in_expr)
	: cexpr(NULL), expr(in_expr)
	{ }

	CondOrExpr(CondExpr* in_cexpr)
	: cexpr(in_cexpr), expr(NULL)
	{ }

	virtual ~CondOrExpr(void)
	{
		if (cexpr != NULL) delete cexpr;
		if (expr != NULL) delete expr;
	}

	const CondExpr* getCondExpr(void) const { return cexpr; }
	const Expr* getExpr(void) const { return expr; }

private:
	CondOrExpr() {}
	CondExpr	*cexpr;
	Expr		*expr;
};

class ExprRewriteVisitor : public ExprVisitor
{
public:
	virtual Expr* applyReplace(Expr* &e)
	{
		Expr	*new_expr = apply(e);
		delete e;
		return new_expr;
	}

	virtual Expr* apply(const Expr* e)
	{
		Expr	*new_expr = e->accept(this);
		if (new_expr == NULL) return e->simplify();
		return new_expr;
	}

	virtual Expr* visit(const Expr* a) {return a->simplify();}
	virtual Expr* visit(const Id* a)  {return a->simplify();}
	virtual Expr* visit(const IdArray* a) {return a->simplify();}
	virtual Expr* visit(const FCall* a) {return a->simplify();}
	virtual Expr* visit(const BinArithOp* a) {return a->simplify();}
	virtual Expr* visit(const IdStruct* a) {return a->simplify();}
	virtual Expr* visit(const ExprParens* a) {return a->simplify();}
	virtual Expr* visit(const CondVal* a) { return a->simplify(); }

	virtual ~ExprRewriteVisitor() {}

protected:
	ExprRewriteVisitor() {}
};

class ExprRewriteAll : public ExprRewriteVisitor
{
public:
	Expr* visit(const BinArithOp* a) override
	{
		const Expr	*lhs, *rhs;
		BinArithOp	*new_bop;
		Expr		*new_lhs, *new_rhs;

		lhs = a->getLHS();
		rhs = a->getRHS();

		new_lhs = apply(lhs);
		new_rhs = apply(rhs);

		new_bop = dynamic_cast<BinArithOp*>(a->copy());
		assert(new_bop != NULL);

		if (new_lhs == lhs && new_rhs == rhs) {
			delete new_lhs;
			delete new_rhs;
			return new_bop;
		}

		if (new_lhs != NULL) new_bop->setLHS(new_lhs);
		if (new_rhs != NULL) new_bop->setRHS(new_rhs);

		return new_bop;
	}

	Expr* visit(const FCall* fc) override {
		auto new_el = new ExprList();
		for (auto &e : *fc->getExprs())
			new_el->add(apply(e.get()));
		return new FCall(new Id(fc->getName()), new_el);
	}

	Expr* visit(const IdArray* ida) override  {
		const Expr	*idx = ida->getIdx();
		return new IdArray(new Id(ida->getName()), apply(idx));
	}

	Expr* visit(const ExprParens* epa) override  {
		return new ExprParens(apply(epa->getExpr()));
	}

	Expr* visit(const CondVal* cv) override {
		return new CondVal(
			cv->getCond()->copy(),
			apply(cv->getTrue()),
			apply(cv->getFalse()));
	}
};

#endif
