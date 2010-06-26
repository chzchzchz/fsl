#include <iostream>

#include "symtab.h"
#include "type.h"

using namespace std;

bool SymbolTable::loadTypeStmt(
	const ptype_map&	tm,
	const Expr*		base_expr,
	const TypeStmt*		stmt)
{
	/* special cases that don't map into symtable  */
	const TypeBlock		*t_block;
	const TypeCond		*t_cond;
	/* cases that do map into the symtable */
	const TypeDecl		*t_decl;
	const TypeParamDecl	*t_pdecl;
	bool			rc;

	t_block = dynamic_cast<const TypeBlock*>(stmt);
	t_cond = dynamic_cast<const TypeCond*>(stmt);
	t_decl = dynamic_cast<const TypeDecl*>(stmt);
	t_pdecl = dynamic_cast<const TypeParamDecl*>(stmt);
	
	rc = true;
	if (t_block != NULL) {
		rc = loadTypeBlock(tm, base_expr, t_block);
	} else if (t_cond != NULL) {
		rc = loadTypeCond(tm, base_expr, t_cond);
	} else if (t_decl != NULL) {
		rc = add(
			t_decl->getName(),
			stmt->resolve(tm),
			base_expr->simplify());
		if (rc == false) {
			cerr	<< "Duplicate symbol: " 
				<< t_decl->getName() << endl;
		}
	} else if (t_pdecl != NULL) {
		rc = add(
			t_pdecl->getName(),
			stmt->resolve(tm),
			base_expr->simplify());
		if (rc == false) {
			cerr	<< "Duplicate symbol: " 
				<< t_pdecl->getName() << endl;
		}
	}

	return rc;
}

bool SymbolTable::loadTypeCond(
	const ptype_map&	tm,
	const Expr*		base_expr,
	const TypeCond*		tc)
{
	const TypeStmt*	stmt_false;
	bool		rc;

	rc = loadTypeStmt(tm, base_expr, tc->getTrueStmt());
	if (rc == false)
		return rc;

	stmt_false = tc->getFalseStmt();
	if (stmt_false != NULL)
		rc = loadTypeStmt(tm, base_expr, stmt_false);

	return rc;
}

/* HACK HACK HACK */
bool SymbolTable::loadTypeBlock(
	const ptype_map&	tm,
	const Expr*	 	base_expr,
	const TypeBlock*	tb) 
{
	TypeBlock::const_iterator	it;
	Expr*				cur_expr;
	bool				rc;

	rc = true;
	cur_expr = base_expr->simplify();

	for (it = tb->begin(); it != tb->end(); it++) {
		TypeStmt	*cur_stmt;
		PhysicalType	*pt;

		cur_stmt = *it;

		rc = loadTypeStmt(tm, cur_expr, cur_stmt);
		if (rc == false)
			break;


		pt = cur_stmt->resolve(tm);
		if (pt == NULL) {
			cerr << "Could not resolve type" << endl;
			rc = false;
			break;
		}

		cur_expr = new AOPAdd(cur_expr, pt->getBits());

		delete pt;
	}

	delete cur_expr;

	return rc;
}

bool SymbolTable::loadArgs(const ptype_map& tm, const ArgsList *args)
{
	if (args == NULL) return true;

	for (unsigned int i = 0; i < args->size(); i++) {
		pair<Id*, Id*>	p;
		PhysicalType	*pt;
		ExprList	*exprs;
		bool		rc;

		p = args->get(i);
		pt = resolve_by_id(tm, p.first);
		if (pt == NULL) return false;

		exprs = new ExprList();
		exprs->add(new Number(i));

		rc = add(
			(p.second)->getName(),
			pt,
			new FCall(new Id("__arg"), exprs));

		if (rc == false) return false;
	}

	return true;
}

