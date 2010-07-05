#include <assert.h>
#include <iostream>

#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "eval.h"

using namespace std;

static Expr* expr_resolve_ids(const EvalCtx& ectx, const Expr* expr);
bool xxx_debug_eval = false;
extern ptype_map ptypes_map;
static Expr* evalReplace(const EvalCtx& ectx, Expr* expr);
static Expr* eval_rewrite_sizeof(const EvalCtx& ectx, const FCall* fc);

static const Type* PT2Type(const PhysicalType* pt)
{
	const PhysTypeUser	*ptu;
	const PhysTypeThunk	*ptthunk;

	ptu = dynamic_cast<const PhysTypeUser*>(pt);
	if (ptu != NULL) {
		return ptu->getType();
	}

	ptthunk = dynamic_cast<const PhysTypeThunk*>(pt);
	if (ptthunk != NULL)
		return ptthunk->getType();

	return NULL;
}


class ExprRewriteConsts : public ExprRewriteAll
{
public:
	ExprRewriteConsts(const const_map& consts)
	: constants(consts) {}

	virtual Expr* visit(const Id* id) 
	{
		Expr	*new_expr;
		new_expr = getNewExpr(id);
		if (new_expr == NULL)
			return id->copy();
		return new_expr;
	}

private:
	Expr* getNewExpr(const Id* id)
	{
		const_map::const_iterator	it;
		
		it = constants.find(id->getName());
		if (it == constants.end())
			return NULL;

		return ((*it).second)->copy();
	}
	
	const const_map& constants;

};

class ExprResolveIds : public ExprRewriteAll
{
public:
	ExprResolveIds(const EvalCtx& evalctx)
		: ectx(evalctx) {}
	
	virtual Expr* visit(const Id* id)
	{
		Expr	*result;
		result = ectx.resolve(id);
		if (result == NULL) 
			return id->copy();
		return result;
	}

	virtual Expr* visit(const IdStruct* ids)
	{	
		Expr	*result;

		result = ectx.resolve(ids);
		if (result == NULL)
			return ids->copy();

		return result;
	}

	virtual Expr* visit(const IdArray* ida)
	{
		Expr	*result;

		result = ectx.resolve(ida);
		if (result == NULL)
			return ida->copy();

		return result;
	}
	
	virtual Expr* visit(const FCall* fc)
	{
		Expr	*new_expr;
		FCall	*new_fc;
		
		if (fc->getName() == "sizeof") {
			return eval_rewrite_sizeof(ectx, fc);
		}

		return ExprRewriteAll::visit(fc);
	}

private:
	const EvalCtx& ectx;
};

static Expr* expr_resolve_ids(const EvalCtx& ectx, const Expr* expr)
{
	ExprResolveIds	eri(ectx);
	return eri.apply(expr);
}


/* return non-null if new expr is allocated to replace cur_expr */
Expr* expr_resolve_consts(const const_map& consts, Expr* cur_expr)
{
	ExprRewriteConsts	erc(consts);
	return erc.apply(cur_expr);
}


static Expr* eval_rewrite_sizeof(const EvalCtx& ectx, const FCall* fc)
{
	const ExprList	*exprs;
	Expr		*front;
	Id		*front_id;
	PhysicalType	*pt;

	exprs = fc->getExprs();
	if (exprs->size() != 1) {
		cerr <<  "sizeof expects 1 argument. Got: ";
		fc->print(cerr);
		cerr << endl;
		return NULL;
	}

	front = exprs->front();
	front_id = dynamic_cast<Id*>(front);
	if (front_id == NULL) {
		cerr << "sizeof expects id for argument. Got: ";
		fc->print(cerr);
		cerr << endl;
		return NULL;
	}

	pt = ptypes_map[front_id->getName()];
	if (pt == NULL) {
		cerr	<< "Could not find type for " << front_id->getName() 
			<< endl;
		return NULL;
	}

	return pt->getBytes();
}

llvm::Value* evalAndGen(const EvalCtx& ectx, const Expr* expr)
{
	Expr		*ret;
	llvm::Value	*v;

	ret = eval(ectx, expr);
	if (ret == NULL)
		return NULL;

	v = ret->codeGen();
	delete ret;

	return v;
}

Expr* eval(const EvalCtx& ectx, const Expr* expr)
{
	Expr	*our_expr, *tmp_expr;
	
	/* first, get simplified copy */
	our_expr = expr->simplify();

	/* resolve all labeled constants into numbers */
	tmp_expr = expr_resolve_consts(ectx.getConstants(), our_expr);
	if (tmp_expr != NULL) {
		delete our_expr;
		our_expr = tmp_expr;
	}

	/* 
	 * resolve all unknown ids into function calls
	 */
	tmp_expr = expr_resolve_ids(ectx, our_expr);
	if (tmp_expr != NULL) {
		delete our_expr;
		our_expr = tmp_expr;
	}

	if (*our_expr != expr) {
		return evalReplace(ectx, our_expr);
	}

	return our_expr;
}

Expr* EvalCtx::resolve(const Id* id) const
{
	const_map::const_iterator	const_it;
	sym_binding			symbind;
	const Type			*t;

	assert (id != NULL);

	/* is it a constant? */
	const_it = constants.find(id->getName());
	if (const_it != constants.end()) {
		return ((*const_it).second)->simplify();
	}
	
	
	/* is is in the current scope? */
	if (cur_scope.lookup(id->getName(), symbind) == true) {
	#if 0
		ExprList	*elist;

		elist = new ExprList();
		elist->add(symbind_off(symbind)->simplify());
		elist->add(symbind_phys(symbind)->getBits());

		return new FCall(new Id("__getLocal"), elist);
	#endif
		return symbind_off(symbind)->simplify();
	}

	/* support for access of dynamic types.. gets base bits for type */
	if ((t = typeByName(id->getName())) != NULL) {
		ExprList	*exprs;

		exprs = new ExprList();
		exprs->add(new Number(t->getTypeNum()));
		return new FCall(new Id("__getDyn"), exprs);
	}

	/* could not resolve */
	return NULL;
}

Expr* EvalCtx::getStructExprBase(
	const SymbolTable		*first_symtab,
	const IdStruct::const_iterator	it_begin,
	const SymbolTable*		&next_symtab) const
{
	const Expr		*first_ids_expr;
	string			first_name;
	Expr			*first_idx;
	sym_binding		sb;
	PhysicalType		*pt;
	PhysTypeArray		*pta;
	const PhysTypeUser	*ptu;
	const Type		*cur_type;
	Expr			*ret;

	first_ids_expr = *it_begin;
	first_idx = NULL;

	if (toName(first_ids_expr, first_name, first_idx) == false) {
		return NULL;
	}

	if (first_symtab->lookup(first_name, sb) == false) {
		if (first_idx != NULL) delete first_idx;
		return NULL;
	}

	ret = new Id(first_name);

	pt = symbind_phys(sb);
	pta = dynamic_cast<PhysTypeArray*>(pt);
	if (pta != NULL) {
		Expr	*tmp_idx;

		/* it's an array */
		if (first_idx == NULL) {
			cerr << "Array type but no index?" << endl;
			delete ret;
			return NULL;
		}

		tmp_idx = new AOPSub(first_idx, new Number(1));
		ret = new AOPAdd(ret, pta->getBits(tmp_idx));
		delete tmp_idx;

		cur_type = PT2Type(pta->getBase());
		ptu = dynamic_cast<const PhysTypeUser*>(pta->getBase());
	} else {
		/* it's a scalar */
		if (first_idx != NULL) {
			cerr << "Tried to index a scalar?" << endl;
			delete first_idx;
			delete ret;
			return NULL;
		}
		cur_type = PT2Type(pt);
	}

	if (cur_type == NULL) {
		cout << "NULL PTU ON ";
		first_ids_expr->print(cout);
		cout << endl;
	}

	assert (cur_type != NULL);
	next_symtab = symtabByName(cur_type->getName());

	return ret;
}

Expr* EvalCtx::getStructExpr(
	const Expr			*base,
	const SymbolTable		*first_symtab,
	const IdStruct::const_iterator	ids_first,
	const IdStruct::const_iterator	ids_end,
	const PhysicalType*		&final_type) const
{
	IdStruct::const_iterator	it;
	Expr				*ret;
	const SymbolTable		*cur_symtab;

	it = ids_first;
	if (base == NULL) {
		ret = getStructExprBase(first_symtab, it, cur_symtab);
		if (ret == NULL) {
			return NULL;
		}
		it++;
	} else {
		ret = base->simplify();
		cur_symtab = first_symtab;
	}


	for (; it != ids_end; it++) {
		const Expr		*cur_expr = *it;
		std::string		cur_name;
		Expr			*cur_idx;
		PhysicalType		*cur_pt;
		PhysTypeArray		*pta;
		sym_binding		sb;
		Expr			*ret_begin;
		const Type		*cur_type;

		ret_begin = ret->simplify();
		if (cur_symtab == NULL) {
			/* no way we could possibly resolve the current 
			 * element. */
			delete ret;
			delete ret_begin;
			return NULL;
		}

		if (toName(cur_expr, cur_name, cur_idx) == false) {
			delete ret;
			delete ret_begin;
			return NULL;
		}

		if (cur_symtab->lookup(cur_name, sb) == false) {
			if (cur_idx != NULL) delete cur_idx;
			delete ret;
			delete ret_begin;
			return NULL;

		}

		cur_pt = symbind_phys(sb);
		pta = dynamic_cast<PhysTypeArray*>(cur_pt);
		if (pta != NULL) {
			/* it's an array */
			if (cur_idx == NULL) {
				cerr << "Array type but no index?" << endl;
				delete ret;
				delete ret_begin;
				return NULL;
			}

			ret = new AOPAdd(ret, symbind_off(sb)->simplify());
			ret = new AOPAdd(ret, pta->getBits(cur_idx));
			delete cur_idx;

			cur_type = PT2Type(pta->getBase());
		} else {
			/* it's a scalar */
			if (cur_idx != NULL) {
				cerr << "Tried to index a scalar?" << endl;
				delete cur_idx;
				delete ret;
				delete ret_begin;
				return NULL;
			}
			ret = new AOPAdd(ret, symbind_off(sb)->simplify());
			cur_type = PT2Type(cur_pt);
		}

		if (cur_type == NULL) {
			cur_symtab = NULL;
			delete ret_begin;
		} else {
			/* don't forget to replace any instances of the 
			 * used type with the name of the field being accessed!
			 * (where did we come from and where are we going?)
			 */

			ret = Expr::rewriteReplace(
				ret, 
				new Id(cur_type->getName()),
				ret_begin);

			cur_symtab = symtabByName(cur_type->getName());
		}

		final_type = cur_pt;
	}

	return ret;
}

Expr* EvalCtx::resolve(const IdStruct* ids) const
{	
	Expr				*offset;
	const PhysicalType		*ids_pt;
	const Id			*front_id;
	IdStruct::const_iterator	it;

	assert (ids != NULL);

	it = ids->begin();
	assert (it != ids->end());

	/* in current scope? */
	offset = getStructExpr(
		NULL,
		&cur_scope, ids->begin(), ids->end(), ids_pt);
	if (offset != NULL) {
		ExprList		*exprs;
		const PhysTypeArray	*pta;

		pta = dynamic_cast<const PhysTypeArray*>(ids_pt);

		exprs = new ExprList();
		exprs->add(offset->simplify());

		if (pta != NULL)
			exprs->add((pta->getBase())->getBits());
		else
			exprs->add(ids_pt->getBits());

		delete offset;

		return new FCall(new Id("__getLocal"), exprs);
	}

	/* global call? */
	it = ids->begin();
	front_id = dynamic_cast<const Id*>(*it);
	if (front_id != NULL) {
		const SymbolTable	*top_symtab;
		const Type		*top_type;
		Expr			*base;
		ExprList		*base_exprs;


		top_type = typeByName(front_id->getName());
		top_symtab = symtabByName(front_id->getName());
		if (top_symtab == NULL || top_type == NULL)
			goto err;

		base_exprs = new ExprList();
		base_exprs->add(new Number(top_type->getTypeNum()));
		base = new FCall(new Id("__getDyn"), base_exprs);

		it++;
		offset = getStructExpr(
			base, top_symtab, it, ids->end(), ids_pt);
		delete base;
		if (offset != NULL) {
			ExprList		*exprs;
			const PhysTypeArray	*pta;

			pta = dynamic_cast<const PhysTypeArray*>(ids_pt);

			exprs = new ExprList();
			exprs->add(offset->simplify());
			if (pta != NULL)
				exprs->add((pta->getBase())->getBits());
			else
				exprs->add(ids_pt->getBits());

			delete offset;
			return new FCall(new Id("__getLocal"), exprs);
		}
	}

err:
	/* can't resolve it.. */
	cerr << "Can't resolve idstruct: ";
	ids->print(cerr);
	cerr << endl;

	return NULL;
}

Expr* EvalCtx::resolve(const IdArray* ida) const
{
	const_map::const_iterator	const_it;
	sym_binding			symbind;
	const PhysicalType		*pt;
	const PhysTypeArray		*pta;

	assert (ida != NULL);

	if (cur_scope.lookup(ida->getName(), symbind) == true) {
		/* array is in current scope */
		ExprList	*elist;
		Expr		*evaled_idx;

		ida->getIdx()->print(cout);
		cout << endl;

		evaled_idx = eval(*this, ida->getIdx());

		if (evaled_idx == NULL) {
			cerr << "Could not resolve ";
			(ida->getIdx())->print(cerr);
			cerr << endl;
			return NULL;
		}

		pt = symbind_phys(symbind);
		assert (pt != NULL);

		pta = dynamic_cast<const PhysTypeArray*>(pt);
		assert (pta != NULL);


		/* convert into __getLocalArray call */
		elist = new ExprList();
		elist->add(evaled_idx->simplify());
		elist->add((pta->getBase())->getBits());
		elist->add(symbind_off(symbind)->simplify());
		elist->add(symbind_phys(symbind)->getBits());

		delete evaled_idx;

		return new FCall(new Id("__getLocalArray"), elist);
	}
	
/* TODO -- add support for constant arrays */
	assert (0 == 1);
#if 0
	const_it = constants.find(id->getName());
	if (const_it != constants.end()) {
		return (const_it.second)->simplify();
	}
#endif
	
	
	/* could not resolve */
	return NULL;
}

bool EvalCtx::toName(const Expr* e, std::string& in_str, Expr* &idx) const
{
	const Id		*id;
	const IdArray		*ida;
	
	idx = NULL;
	id = dynamic_cast<const Id*>(e);
	if (id != NULL) {
		in_str = id->getName();
		return true;
	}

	ida = dynamic_cast<const IdArray*>(e);
	if (ida != NULL) {
		in_str = ida->getName();
		idx = (ida->getIdx())->simplify();
		return true;
	}

	return false;
}

const SymbolTable* EvalCtx::symtabByName(const std::string& s) const
{
	symtab_map::const_iterator	it;

	it = all_types.find(s);
	if (it == all_types.end())
		return NULL;

	return (*it).second;
}

const Type* EvalCtx::typeByName(const std::string& s) const
{
	const SymbolTable	*st;
	const Type		*t;
	const PhysicalType	*pt;
	const PhysTypeUser	*ptu;

	st = symtabByName(s);
	if (st == NULL)
		return NULL;

	pt = st->getOwner();
	assert (pt != NULL);

	ptu = dynamic_cast<const PhysTypeUser*>(pt);
	if (ptu == NULL)
		return NULL;

	return ptu->getType();
}

static Expr* evalReplace(const EvalCtx& ectx, Expr* expr)
{
	Expr	*new_expr;

	if (expr == NULL) return NULL;

	new_expr = eval(ectx, expr);
	delete expr;
	return new_expr;
}

