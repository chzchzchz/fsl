#ifndef EXPR_H
#define EXPR_H

#include <map>
#include "llvm/DerivedTypes.h"
#include "collection.h"

typedef std::map<std::string, class Expr*>		const_map;
typedef std::map<const class Expr, const class Expr*>	rewrite_map;

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

#if 0
	virtual bool operator<(const Expr* e) const
	{
		if (this == e) {
			return false;
		}

		if (typeid(this) == typeid(e)) 
			return (*this < e);
		else if (typeid(this) < typeid(e))
			return true;
		else
			return false;
	}
#endif

	virtual Expr* rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		if (*this == to_rewrite) {
			return new_expr->simplify();
		}

		return NULL;
	}

	virtual llvm::Value* codeGen() const = 0;

	llvm::Value* ErrorV(const char* s) const;
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

	void rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		for (iterator it = begin(); it != end(); it++) {
			Expr	*cur_expr, *tmp_expr;

			cur_expr = *it;

			tmp_expr = cur_expr->rewrite(to_rewrite, new_expr);
			if (tmp_expr != NULL) {
				(*it) = tmp_expr;
				delete cur_expr;
				cur_expr = tmp_expr;
			}
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

		return (*(in_fc->id) == id) && (*exprs == in_fc->exprs);
	}

	virtual Expr* rewrite(const Expr* to_rewrite, const Expr* new_expr)
	{
		exprs->rewrite(to_rewrite, new_expr);
		return Expr::rewrite(to_rewrite, new_expr);
	}

#if 0
	bool operator<(const Expr* e) const
	{
		FCall *fc;

		fc = dynamic_cast<const FCalL*>(e);
		if (fc == NULL) {
			return Expr::operator<(e);
		}

		if (*id != fc->id) {
			return (*id < fc->id);
		}


		assert (0 == 1);
		return id_name < cmp_id.id_name
	}
#endif
	llvm::Value*  codeGen() const;
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
private:
	unsigned long n;
};

#include "binarithop.h"
#include "cond_expr.h"

#endif
