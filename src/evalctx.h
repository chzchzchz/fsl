#ifndef EVALCTX_H
#define EVALCTX_H

#include "symtab.h"
#include "type.h"
#include "args.h"
#include "expr.h"
#include "func.h"

struct TypeBase
{
	/* gives sym we're sitting on  */
	const Type		*tb_type;
	const SymbolTable	*tb_symtab;
	const SymbolTableEnt	*tb_lastsym;
	/* expressions that'll take us to sym */
	Expr			*tb_diskoff;
	Expr			*tb_parambuf;
	Expr			*tb_virt;
};

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

	bool resolveTB(
		const IdStruct* ids, struct TypeBase& tb,
		Expr* &parent_closure) const;

	Expr* resolveArrayInType(const IdArray* ida) const;
	bool setNewOffsets(
		struct TypeBase& current_base, const Expr* idx) const;

	bool resolveIdStructCurScope(
		const IdStruct* ids,
		const std::string& name,
		const Expr* idx,
		struct TypeBase& tb) const;

	bool resolveIdStructFunc(
		const IdStruct* ids,
		const std::string& name,
		struct TypeBase& tb) const;

	bool resolveIdStructFHead(
		const IdStruct* ids,
		struct TypeBase& tb,
		Expr* &parent_clo) const;

	bool resolveIdStructVarScope(
		const IdStruct* ids,
		const std::string& name,
		const VarScope* vs,
		struct TypeBase& tb) const;

	Expr* setNewOffsetsArray(
		Expr* new_base,
		Expr** new_params,
		const struct TypeBase& tb,
		const ThunkField* tf,
		const Expr* idx) const;

protected:
	const FuncBlock*	cur_func_blk;
	const Func*		cur_func;
	const SymbolTable*	cur_scope;
	const VarScope*		cur_vscope;

	const SymbolTable*	symtabByName(const std::string& s) const;
	const Type*		typeByName(const std::string& s) const;


	bool resolveTail(
		TypeBase			&tb,	/* current base */
		const IdStruct* 		ids,
		IdStruct::const_iterator	ids_begin) const;

};

#endif
