#ifndef EXPR_H
#define EXPR_H

#include <iostream>
#include <map>
#include "llvm/DerivedTypes.h"
#include "collection.h"

typedef std::map<std::string, class Expr*>		const_map;
typedef std::map<const class Expr, const class Expr*>	rewrite_map;

class Id;
class IdArray;
class FCall;
class BinArithOp;
class IdStruct;
class ExprParens;

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
	bool operator!=(const Expr* e) const
	{
		return ((*this == e) == false);
	}

	virtual Expr* rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		if (*this == to_rewrite) {
			return new_expr->simplify();
		}

		return NULL;
	}

	/**
	 *
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
		
		rewritten_expr = e->rewrite(to_rewrite, new_expr);
		delete to_rewrite;
		delete new_expr;
		if (rewritten_expr == NULL)
			return e;

		delete e;
		return rewritten_expr;;
	}

	virtual llvm::Value* codeGen() const = 0;

	llvm::Value* ErrorV(const std::string& s) const;

	virtual Expr* accept(ExprVisitor* ev) const = 0;
protected:
	Expr() {}
};

class ExprList : public PtrList<Expr>
{
public:
	ExprList() {}
	ExprList(Expr* e) { assert (e != NULL); add(e); }

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

	void rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		for (iterator it = begin(); it != end(); it++) {
			Expr	*cur_expr, *tmp_expr;

			cur_expr = *it;

			tmp_expr = cur_expr->rewrite(to_rewrite, new_expr);
			if (tmp_expr == NULL)
				continue;


			(*it) = tmp_expr;
			delete cur_expr;
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

		if (new_exprs == exprs) return;

		delete exprs;
		exprs = new_exprs->simplify();
		delete new_exprs;
	}

	const std::string& getName(void) const { return id->getName(); }
	void print(std::ostream& out) const
	{
		out << "[ fcall " << getName() << "("; 
		exprs->print(out);
		out << ") ]";
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

		return (*(in_fc->id) == id) && (*exprs == in_fc->exprs);
	}

	virtual Expr* rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		exprs->rewrite(to_rewrite, new_expr);
		return Expr::rewrite(to_rewrite, new_expr);
	}

	llvm::Value*  codeGen() const;

	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
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
		it++;

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

	llvm::Value* codeGen() const;

	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
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

	llvm::Value* codeGen() const;

	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
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
		if (e == expr) return;
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

	llvm::Value* codeGen() const;

	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
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

	llvm::Value* codeGen() const;


	virtual Expr* accept(ExprVisitor* ev) const { return ev->visit(this); }
private:
	unsigned long n;
};


#include "binarithop.h"
#include "cond_expr.h"

class CondOrExpr {
public:
	CondOrExpr(Expr* in_expr)
	: cexpr(NULL), expr(in_expr)
	{
		assert (expr != NULL);
	}

	CondOrExpr(CondExpr* in_cexpr)
	: cexpr(in_cexpr), expr(NULL)
	{
		assert (cexpr != NULL);
	}

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
		Expr	*new_expr;

		new_expr = apply(e);
		delete e;

		return new_expr;
	}

	virtual Expr* apply(const Expr* e)
	{
		Expr	*new_expr;

		new_expr = e->accept(this);
		if (new_expr == NULL) {
			return e->simplify();
		}

		return new_expr;
	}

	virtual Expr* visit(const Expr* a) {return a->simplify();}
	virtual Expr* visit(const Id* a)  {return a->simplify();}
	virtual Expr* visit(const IdArray* a) {return a->simplify();}
	virtual Expr* visit(const FCall* a) {return a->simplify();}
	virtual Expr* visit(const BinArithOp* a) {return a->simplify();}
	virtual Expr* visit(const IdStruct* a) {return a->simplify();}
	virtual Expr* visit(const ExprParens* a) {return a->simplify();}


	virtual ~ExprRewriteVisitor() {}

protected:
	ExprRewriteVisitor() {}
};

class ExprRewriteAll : public ExprRewriteVisitor
{
public:
	virtual Expr* visit(const BinArithOp* a)
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

	virtual Expr* visit(const FCall* fc)
	{
		ExprList        *new_el;
		const ExprList  *old_el;

		old_el = fc->getExprs();
		new_el = new ExprList();
		for (   ExprList::const_iterator it = old_el->begin();
			it != old_el->end();
			it++)
		{
			Expr*		new_expr;
			const Expr*	arg_expr;

			arg_expr = *it;
			new_expr = apply(arg_expr);
			new_el->add(new_expr);
		}

		return new FCall(new Id(fc->getName()), new_el);
	}


	virtual Expr* visit(const IdArray* ida)
	{
		const Expr	*idx;
		idx = ida->getIdx();
		return new IdArray(new Id(ida->getName()), apply(idx));
	}

	virtual Expr* visit(const ExprParens* epa)
	{
		return new ExprParens(apply(epa->getExpr()));
	}
};




#endif
