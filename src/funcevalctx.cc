#include "funcevalctx.h"

using namespace std;

bool FuncEvalCtx::addTailScalar(
	const PhysicalType	*cur_pt,
	const SymbolTable*	cur_symtab,
	const string		&cur_name,
	Expr*			&cur_idx,
	Expr*			&ret) const
{
	ExprList	*exprs;

	if (cur_idx != NULL) {
		cerr << "Tried to index a scalar. Bail" << endl;
		return false;
	}

	exprs = new ExprList();
	exprs->add(ret);
	ret = new FCall(
		new Id(typeOffThunkName(
			cur_symtab->getOwnerType(),
			cur_name)),
		exprs);

	return true;
}

bool FuncEvalCtx::addTailArray(
	const PhysTypeArray* 	pta,
	const SymbolTable*	cur_symtab,
	const string		&cur_name,
	Expr*			&cur_idx,
	Expr*			&ret) const
{
	ExprList	*exprs;

	assert (pta != NULL);

	if (cur_idx == NULL) {
		cerr << "Array type but no index?" << endl;
		/* cleanup */
		return false;
	}

	exprs = new ExprList();
	exprs->add(ret);

	ret = new FCall(
		new Id(typeOffThunkName(cur_symtab->getOwnerType(), cur_name)),
		exprs);

	assert (pta->getBase() != NULL);
#if 0
	if (pta->getBase() == NULL) {
		/* XXX when does this happen??? */
		assert (0 == 1);
		ret = new AOPAdd(ret, pta->getBits(cur_idx));
	} else { ...
#endif
	/* force thunk when calculating length  */
	exprs = new ExprList();
	exprs->add(ret->simplify());

	ret = new AOPAdd(
		ret,
		new AOPMul(
			cur_idx->simplify(),
			new FCall(
				new Id(typeThunkName((pta->getBase())->getName())),
				exprs)
		)
	);

	delete cur_idx;
	cur_idx = NULL;

	return true;
}

Expr* FuncEvalCtx::buildTail(
	IdStruct::const_iterator	it,
	IdStruct::const_iterator	ids_end,
	Expr				*ret,
	const SymbolTable		*parent_symtab,
	const PhysicalType*		&final_type) const
{
	const Expr			*cur_ids_expr;
	std::string			cur_name;
	const PhysicalType		*cur_pt;
	const PhysicalType		*pt_base;
	const PhysTypeArray		*pta;
	const SymbolTableEnt		*st_ent;
	const Type			*cur_type;
	Expr				*cur_idx = NULL;
	const SymbolTable		*cur_symtab;

	if (it == ids_end)
		return ret;

	if (parent_symtab == NULL) {
		/* no way we could possibly resolve the current element---
		 * this id struct is likely bogus */
		cerr << "NO PARENT SYMTAB FOUND!" << endl;
		goto err_cleanup;
	}

	cur_ids_expr = *it;

	/* scoop out current name of Id/IdArray in struct and index */
	if (toName(cur_ids_expr, cur_name, cur_idx) == false) {
		cerr << "COULD NOT GET THE NAME OF CURRENT ID: ";
		cur_ids_expr->print(cerr);
		cerr << endl;
		goto err_cleanup;
	}

	/* verify whether the parent struct contains the field given by
	 * the current idstruct entry (and find out the type) */
	if ((st_ent = parent_symtab->lookup(cur_name)) == NULL) {
		cerr	<< "COULD NOT FIND " << cur_name << " IN PARENT SYMTAB "
			<< parent_symtab->getOwner()->getName() << endl;
		goto err_cleanup;
	}

	cur_pt = st_ent->getPhysType();
	cur_type = PT2UserTypeDrill(cur_pt);
	pt_base = PT2Base(cur_pt);

	/* for (REST OF IDS).(parent).(it) we want to compute 
	 * &(REST OF IDS) + offset(REST OF IDS, parent) + offset(parent, it)
	 * currently we only have
	 * ret = &(REST OF IDS) + offset(REST OF IDS, parent) 
	 * so we do
	 * ret += offset(parent, it)
	 * in this stage
	 */
	pta = dynamic_cast<const PhysTypeArray*>(cur_pt);
	if (pta != NULL) {
		if (addTailArray(pta, parent_symtab, cur_name, cur_idx, ret) == false) {
			cerr << "COULD NOT ADD TAIL ARRAY" << endl;
			goto err_cleanup;
		}
	} else {
		if (addTailScalar(
			cur_pt, parent_symtab, cur_name, cur_idx, ret) == false) {
			cerr << "COULD NOT ADD TAIL SCALAR" << endl;
			goto err_cleanup;
		}
	}
	assert (cur_idx == NULL);

	final_type = cur_pt;
	cur_symtab = symtabByName(pt_base->getName());

	/* remember the last type accessed so that we can load it into memory */
	final_type = cur_pt;

done:
	assert (cur_idx == NULL);
	return buildTail(++it, ids_end, ret, cur_symtab, final_type);

err_cleanup:
	cerr << "FUNCRESOLVE ERROR!" << endl;
	if (ret != NULL) delete ret;
	if (cur_idx != NULL) delete cur_idx;
	return NULL;
}
