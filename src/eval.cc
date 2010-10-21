#include <assert.h>
#include <iostream>

#include <stdint.h>
#include <llvm/DerivedTypes.h>
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/IRBuilder.h>

#include "eval.h"
#include "func.h"
#include "runtime_interface.h"

using namespace std;

extern symtab_map	symtabs;
extern func_map		funcs_map;
extern const_map	constants;
extern RTInterface	rt_glue;

static Expr* expr_resolve_ids(const EvalCtx& ectx, const Expr* expr);
static Expr* eval_rewrite_sizeof(const EvalCtx& ectx, const FCall* fc, bool bits);

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
		if (fc->getName() == "sizeof_bits") {
			Expr	*ret;
			ret = eval_rewrite_sizeof(ectx, fc, true);
			return ret;
		} else if (fc->getName() == "sizeof_bytes") {
			Expr*	ret;
			ret = eval_rewrite_sizeof(ectx, fc, false);
			return ret;
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

static Expr* eval_rewrite_sizeof(const EvalCtx& ectx, const FCall* fc, bool bits)
{
	const ExprList			*exprs;
	const SymbolTable		*st;
	const Type			*t;
	Expr				*front;
	Expr				*ret_size;
	Id				*front_id;
	symtab_map::const_iterator	it;

	/* fc name is either sizeof_bytes of sizeof_bits */
	/* should probably make this its own class? */

	exprs = fc->getExprs();
	if (exprs->size() != 1) {
		cerr <<  "sizeof expects 1 argument. Got: ";
		fc->print(cerr);
		cerr << endl;
		return NULL;
	}

	/* sizeof currently only takes a type */
	front = exprs->front();
	front_id = dynamic_cast<Id*>(front);
	if (front_id == NULL) {
		/* TODO: should take re-parameterized types and funcs too.. */
		cerr << "sizeof expects id for argument. Got: ";
		fc->print(cerr);
		cerr << endl;
		return NULL;
	}

	/* find symtab that gives matching name */
	it = symtabs.find(front_id->getName());
	if (it == symtabs.end()) {
		cerr	<< "Could not find type for " 
			<< front_id->getName() 
			<< endl;
		return NULL;
	}

	/* st = symbol table for type to get size of */
	st = (*it).second;

	/* type of symbol table */
	t = st->getOwnerType();

	/* get typified size expression,
	 * fill in closure parameter with the dynamic type */
	ret_size = st->getThunkType()->getSize()->copyFCall();
	ret_size = Expr::rewriteReplace(
		ret_size,
		rt_glue.getThunkClosure(),
		rt_glue.getDynClosure(t));

	if (bits == false) {
		/* convert bits to bytes */
		ret_size = new AOPDiv(ret_size, new Number(8));
	}

	return ret_size;
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
	tmp_expr = expr_resolve_consts(constants, our_expr);
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

	tmp_expr = expr_resolve_consts(constants, our_expr);
	if (tmp_expr != NULL) {
		delete our_expr;
		our_expr = tmp_expr;
	}

	if (*our_expr != expr) {
		our_expr = evalReplace(ectx, our_expr);
	}

	return our_expr;
}

Expr* evalReplace(const EvalCtx& ectx, Expr* expr)
{
	Expr	*new_expr;

	if (expr == NULL) return NULL;

	new_expr = eval(ectx, expr);
	delete expr;
	return new_expr;
}

