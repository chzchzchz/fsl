#ifndef EVALCTX_H
#define EVALCTX_H

#include <stack>
#include "symtab.h"
#include "type.h"
#include "args.h"
#include "expr.h"
#include "func.h"
#include "type_stack.h"

/**
 * will eventualy want to replace this with different eval contexts so
 * that we can use it for handling local scopes within functions
 */
class EvalCtx
{
public:
	/* evaluating for a type */
	EvalCtx(const SymbolTable* in_cur_scope)
	: cur_func_blk(NULL),
	  cur_func(NULL),
	  cur_scope(in_cur_scope),
	  cur_vscope(NULL)
	{
		assert (in_cur_scope != NULL);
	}

	EvalCtx(const class VarScope* vs)
	: cur_func_blk(NULL),
	  cur_func(NULL),
	  cur_scope(NULL),
	  cur_vscope(vs)
	{
		assert (vs != NULL);
	}

	/* evaluating for a function */
	EvalCtx(const FuncBlock* in_func)
	:	cur_func_blk(in_func),
		cur_scope(NULL),
		cur_vscope(NULL)
	{
		assert (cur_func_blk != NULL);
		cur_func = cur_func_blk->getFunc();
		assert (cur_func != NULL);
	}

	virtual ~EvalCtx() {}

	/* given an expression that is either a scalar id or an array id,
	 * return the name and (if applicable) the index into the type
	 */
	bool toName(const Expr* e, std::string& in_str, Expr* &idx) const;

	Expr* resolveVal(const Id* id) const;
	Expr* resolveVal(const IdArray* ida) const;
	Expr* resolveVal(const IdStruct* ids) const;

	Expr* resolveLoc(const IdStruct* idsm, Expr* &size) const;

	const Type* getType(const Expr* e) const;
	const Type* getTypeId(const Id* id) const;
	const Type* getTypeIdStruct(const IdStruct* id) const;

private:
	EvalCtx(void);

	TypeStack* resolveTail(
		TypeBase			*head,
		const IdStruct* 		ids,
		IdStruct::const_iterator	ids_begin) const;

	TypeStack* resolveTypeStack(
		const IdStruct* ids, Expr* &parent_closure) const;
	Expr* resolveArrayInType(const IdArray* ida) const;

	TypeStack* resolveIdStructCurScope(
		const IdStruct* ids,
		const std::string& name,
		const Expr* idx) const;

	TypeStack* resolveIdStructFunc(
		const IdStruct* ids,
		const std::string& name) const;

	TypeStack* resolveIdStructFHead(
		const IdStruct* ids, Expr* &parent_closure) const;

	TypeStack* resolveIdStructVarScope(
		const IdStruct* ids,
		const std::string& name,
		const VarScope* vs) const;

protected:
	const FuncBlock*	cur_func_blk;
	const Func*		cur_func;
	const SymbolTable*	cur_scope;
	const VarScope*		cur_vscope;

	const Type*		typeByName(const std::string& s) const;

};

#endif
