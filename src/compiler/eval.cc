#include <assert.h>
#include <iostream>

#include <stdint.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/IRBuilder.h>

#include "eval.h"
#include "func.h"
#include "runtime_interface.h"

using namespace std;

extern type_map		types_map;
extern symtab_map	symtabs;
extern ctype_map	ctypes_map;
extern func_map		funcs_map;
extern const_map	constants;
extern RTInterface	rt_glue;

static Expr* expr_resolve_ids(const EvalCtx& ectx, const Expr* expr);
static Expr* eval_rewrite_sizeof(const EvalCtx&, const FCall* fc, bool bits);

class ExprRewriteConsts : public ExprRewriteAll
{
public:
	ExprRewriteConsts(const const_map& consts)
	: constants(consts) {}

	Expr* visit(const Id* id) override  {
		Expr	*new_expr;
		new_expr = getNewExpr(id);
		return (new_expr == NULL) ? id->copy() : new_expr;
	}

private:
	Expr* getNewExpr(const Id* id)
	{
		const_map::const_iterator	it;

		it = constants.find(id->getName());
		if (it == constants.end()) return NULL;
		return ((*it).second)->copy();
	}

	const const_map& constants;
};

class ExprResolveIds : public ExprRewriteAll
{
public:
	ExprResolveIds(const EvalCtx& evalctx)
		: ectx(evalctx) {}

	Expr* visit(const Id* id) override  {
		Expr	*result = ectx.resolveVal(id);
		return (result == NULL) ? id->copy() : result;
	}

	Expr* visit(const IdStruct* ids) override {
		Expr	*result = ectx.resolveVal(ids);
		return (result == NULL) ? ids->copy() : result;
	}

	Expr* visit(const IdArray* ida) override {
		Expr	*result = ectx.resolveVal(ida);
		return (result == NULL) ? ida->copy() : result;
	}

	Expr* visit(const FCall* fc) override {
		if (fc->getName() == "sizeof_bits")
			return eval_rewrite_sizeof(ectx, fc, true);
		else if (fc->getName() == "sizeof_bytes")
			return eval_rewrite_sizeof(ectx, fc, false);
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

static Expr* eval_sizeof_type(const EvalCtx& ectx, const Id* id)
{
	Expr			*ret_size;
	Expr			*resolved_closure;
	const SymbolTable	*st;
	const Type		*t;

	if (id == NULL) return nullptr;

	if (types_map.count(id->getName())) {
		t = types_map[id->getName()];
		assert (t != nullptr);
		resolved_closure = FCall::mkBaseClosure(t);
	} else {
		t = ectx.getType(id);
		if (!t) return nullptr;
		resolved_closure = ectx.resolveVal(id);
		if (!resolved_closure) return nullptr;
	}

	/* st = symbol table for type to get size of */
	st = symtabs[t->getName()];

	/* get typified size expr,  fill in closure param with dynamic type */
	ret_size = st->getThunkType()->getSize()->copyFCall();
	ret_size = Expr::rewriteReplace(
		ret_size, rt_glue.getThunkClosure(), resolved_closure);
	return ret_size;
}

static Expr* eval_sizeof_typedef(const Id* id)
{
	if (ctypes_map.count(id->getName()) == 0) return NULL;
	return new Number(ctypes_map[id->getName()]);
}

static Expr* eval_rewrite_sizeof(const EvalCtx& ectx, const FCall* fc, bool bits)
{
	const ExprList	*exprs;
	Expr		*ret_size;

	/* fc name is either sizeof_bytes of sizeof_bits */
	/* should probably make this its own class? */
	exprs = fc->getExprs();
	if (exprs->size() != 1) {
		cerr <<  "sizeof expects 1 argument. Got: ";
		fc->print(cerr);
		cerr << endl;
		return NULL;
	}

	auto front_id = dynamic_cast<const Id*>(exprs->front().get());
	if (front_id == NULL) {
		cerr << "sizeof expects id for argument. Got: ";
		fc->print(cerr);
		cerr << endl;
		return NULL;
	}

	ret_size = eval_sizeof_type(ectx, front_id);
	if (ret_size == NULL) ret_size = eval_sizeof_typedef(front_id);
	if (ret_size == NULL) {
		cerr << "sizeof couldn't figure out size of '";
		fc->print(cerr);
		cerr << "'\n";;
		return NULL;
	}

	/* convert bits to bytes */
	if (bits == false) ret_size = new AOPDiv(ret_size, new Number(8));
	return ret_size;
}

llvm::Value* evalAndGen(const EvalCtx& ectx, const Expr* expr)
{
	Expr		*ret;
	llvm::Value	*v;

	ret = eval(ectx, expr);
	if (ret == NULL) return NULL;

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

	/* resolve all unknown ids into function calls */
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

	if (*our_expr != expr) our_expr = evalReplace(ectx, our_expr);

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
