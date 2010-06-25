#ifndef EVAL_H
#define EVAL_H

#include "symtab.h"
#include "phys_type.h"
#include "type.h"
#include "expr.h"

/**
 * will eventualy want to replace this with different eval contexts so
 * that we can use it for handling local scopes within functions
 */
class EvalCtx
{
public:
	EvalCtx(
		const SymbolTable&	in_cur_scope,
		const symtab_map&	in_all_types,
		const const_map&	in_constants)
	: cur_scope(in_cur_scope),
	  all_types(in_all_types),
	  constants(in_constants)
	{
	}

	virtual ~EvalCtx() {}

	const SymbolTable&	getCurrentScope() const { return cur_scope; }
	const symtab_map&	getGlobalScope() const { return all_types; }
	const const_map&	getConstants() const { return constants; }

	Expr* resolve(const Id* id) const
	{
		const_map::const_iterator	const_it;
		sym_binding			symbind;

		assert (id != NULL);

		const_it = constants.find(id->getName());
		if (const_it != constants.end()) {
			return ((*const_it).second)->simplify();
		}
		
		
		if (cur_scope.lookup(id->getName(), symbind) == true) {
			ExprList	*elist;

			elist = new ExprList();
			elist->add(symbind_off(symbind)->simplify());
			elist->add(symbind_phys(symbind)->getBits());

			return new FCall(new Id("__getLocal"), elist);
		}
		

		/* could not resolve */
		return NULL;
	}

	bool toName(const Expr* e, std::string& in_str, Expr* &idx) const
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

	Expr* resolve(const IdStruct* ids) const
	{
		IdStruct::const_iterator	it;
		Expr				*ret;
		SymbolTable			*cur_type;
		const Expr			*cur_ids_expr;
		Expr				*cur_idx;
		std::string			cur_name;

		const PhysicalType		*pt;
		const PhysTypeUser		*ptu;

		assert (ids != NULL);

		it = ids->begin();
		assert (it != ids->end());

		cur_ids_expr = *it;
		if (toName(cur_ids_expr, cur_name, cur_idx) == false)
			return NULL;

		/* should not have an array for top-level request. */
		assert (cur_idx == NULL);

		/* get top level symtable so we can start resolving */
		cur_type = (*(all_types.find(cur_name))).second;
		if (cur_type == NULL) {
			return NULL;
		}

		pt = cur_type->getOwner();
		assert (pt != NULL);

		ptu = dynamic_cast<const PhysTypeUser*>(pt);
		if (ptu == NULL) {
			/* Oops! should be user-type */
			return NULL;
		}


		/* XXX -- STUB */
		assert (0 == 1);
		return NULL;
//return new FCall(
//			new Id("__getDyn"), 
//			ptu->getType()->getTypeNum()

	}

	Expr* resolve(const IdArray* ida) const
	{
		const_map::const_iterator	const_it;
		sym_binding			symbind;
		const PhysicalType		*pt;
		const PhysTypeArray		*pta;

		assert (ida != NULL);

	/* TODO -- add support for constant arrays */
	#if 0
		const_it = constants.find(id->getName());
		if (const_it != constants.end()) {
			return (const_it.second)->simplify();
		}
	#endif
		
		
		if (cur_scope.lookup(ida->getName(), symbind) == true) {
			ExprList	*elist;

			pt = symbind_phys(symbind);
			assert (pt != NULL);

			pta = dynamic_cast<const PhysTypeArray*>(pt);
			assert (pta != NULL);


			elist = new ExprList();
			elist->add((ida->getIdx())->simplify());
			elist->add((pta->getBase())->getBits());
			elist->add(symbind_off(symbind)->simplify());
			elist->add(symbind_phys(symbind)->getBits());

			return new FCall(new Id("__getLocalArray"), elist);
		}
		

		/* could not resolve */
		return NULL;
	}

private:
	EvalCtx(void);

	/* do we need to know incoming from_base? */
	const SymbolTable&	cur_scope;
	const symtab_map&	all_types;
	const const_map&	constants;
};

Expr* expr_resolve_consts(const const_map& consts, Expr* cur_expr);
void eval(const EvalCtx& ec, const Expr*);




#endif
