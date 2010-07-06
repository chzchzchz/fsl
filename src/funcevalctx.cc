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
	Expr				*ret;
	const SymbolTable		*cur_symtab;


	it = ids_first;
	cerr << "OH, HELLO. \"";
	(*it)->print(cerr);
	cerr << "\"" << endl;

	if (base == NULL) {
		cerr << "NO BASE!" << endl;
		ret = getStructExprBase(first_symtab, it, cur_symtab);
		if (ret == NULL) {
			cerr << "OOPS. BYE" << endl;
			return NULL;
		}
		it++;
	} else {
		cerr << "HAS BASE! ";
		base->print(cerr);
		cerr << endl;

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

		cerr << "THE NAME IS " << cur_name << endl;

		cur_pt = symbind_phys(sb);
		pta = dynamic_cast<PhysTypeArray*>(cur_pt);
		if (pta != NULL) {
			ExprList	*exprs;
			/* it's an array */
			if (cur_idx == NULL) {
				cerr << "Array type but no index?" << endl;
				delete ret;
				delete ret_begin;
				return NULL;
			}

			exprs = new ExprList();
			exprs->add(ret);

			cerr << "CHECK THIS ARRAY EXPR: ";
			ret->print(cerr);
			cerr << endl;

			ret = new FCall(
				new Id(typeOffThunkName(
					cur_symtab->getOwnerType(),
					cur_name)),
				exprs);
			ret = new AOPAdd(ret, pta->getBits(cur_idx));

			cerr << "OUR CUR IDX WAS ";
			cur_idx->print(cerr);
			cerr << endl;

			Expr	*tmp_e;
			tmp_e = pta->getBits(cur_idx);
			cerr << "THE PTA->GETBITS() = ";
			tmp_e->print(cerr);
			cerr << endl;


			delete cur_idx;

			cerr << "UPDATED TO: ";
			ret->print(cerr);
			cerr << endl;

			cur_type = PT2Type(pta->getBase());
		} else {
			ExprList	*exprs;

			/* it's a scalar */
			if (cur_idx != NULL) {
				cerr << "Tried to index a scalar?" << endl;
				delete cur_idx;
				delete ret;
				delete ret_begin;
				return NULL;
			}

			exprs = new ExprList();
			exprs->add(ret);
			ret = new FCall(
				new Id(typeOffThunkName(
					cur_symtab->getOwnerType(),
					cur_name)),
				exprs);
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

