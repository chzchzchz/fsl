#include "funcevalctx.h"

using namespace std;

Expr* FuncEvalCtx::getStructExpr(
	const Expr			*base,
	const SymbolTable		*first_symtab,
	const IdStruct::const_iterator	ids_first,
	const IdStruct::const_iterator	ids_end,
	const PhysicalType*		&final_type) const
{
	IdStruct::const_iterator	it;
	Expr				*ret = NULL;
	Expr				*cur_idx = NULL;
	const SymbolTable		*cur_symtab;


	cerr << "YOU AND ME." << endl;

	it = ids_first;
	if (base == NULL) {
		ret = getStructExprBase(first_symtab, it, cur_symtab);
		if (ret == NULL) {
			cerr << "COULD NOT FIND BASE" << endl;
			return NULL;
		}
		it++;
	} else {
		cerr << "BASE IS PROVIDED." << endl;
		ret = base->simplify();
		cur_symtab = first_symtab;
	}


	cerr << "FOLLOW YOUR STUPID IDSTRUCT: ";
	(*it)->print(cerr);
	cerr << endl;

	for (; it != ids_end; it++) {
		const Expr		*cur_expr = *it;
		std::string		cur_name;
		PhysicalType		*cur_pt;
		const PhysicalType	*pt_base;
		const PhysTypeUnion	*ptunion;
		PhysTypeArray		*pta;
		sym_binding		sb;
		const Type		*cur_type;

		if (cur_symtab == NULL) {
			/* no way we could possibly resolve the current 
			 * element. */
			goto err_cleanup;
		}

		if (toName(cur_expr, cur_name, cur_idx) == false)
			goto err_cleanup;

		if (cur_symtab->lookup(cur_name, sb) == false)
			goto err_cleanup;

		cur_pt = symbind_phys(sb);
		pta = dynamic_cast<PhysTypeArray*>(cur_pt);
		if (pta != NULL) {
			/* it's an array */

			ExprList	*exprs;

			if (cur_idx == NULL) {
				cerr << "Array type but no index?" << endl;
				goto err_cleanup;
			}

			cur_type = PT2Type(pta->getBase());

			exprs = new ExprList();
			exprs->add(ret);

			ret = new FCall(
				new Id(typeOffThunkName(
					cur_symtab->getOwnerType(),
					cur_name)),
				exprs);

			if (cur_type == NULL) {
				ret = new AOPAdd(ret, pta->getBits(cur_idx));
			} else {
				/* force thunk when calculating length  */
				exprs = new ExprList();
				exprs->add(ret->simplify());
				ret = new AOPAdd(
					ret,
					new AOPMul(
						cur_idx->simplify(),
						new FCall(
						new Id(typeThunkName(cur_type)),
						exprs)
					)
				);
			}

			delete cur_idx;
			cur_idx = NULL;

			pt_base = pta->getBase();
		} else {
			/* it's a scalar */

			ExprList	*exprs;

			if (cur_idx != NULL) {
				cerr << "Tried to index a scalar. Bail" << endl;
				goto err_cleanup;
			}

			pt_base = cur_pt;
			exprs = new ExprList();
			exprs->add(ret);
			ret = new FCall(
				new Id(typeOffThunkName(
					cur_symtab->getOwnerType(),
					cur_name)),
				exprs);
			cur_type = PT2Type(cur_pt);
		}
		assert (cur_idx == NULL);

		ptunion = dynamic_cast<const PhysTypeUnion*>(pt_base);
		if (ptunion != NULL) {
			/* ... hit a union.. what to do here?? */
			assert (0 == 1);
		}

		if (cur_type == NULL) {
			cur_symtab = NULL;
		} else {
			/* move to next symtab */
			cerr << "WHATTTTT." << endl;
			cur_symtab = symtabByName(cur_type->getName());
		}

		final_type = cur_pt;
	}

	assert (cur_idx == NULL);
	return ret;

err_cleanup:
	cerr << "FUNCRESOLVE ERROR!" << endl;
	if (ret != NULL) delete ret;
	if (cur_idx != NULL) delete cur_idx;
	return NULL;
}

