#ifndef EVALCTX_H
#define EVALCTX_H

#include "symtab.h"
#include "type.h"
#include "expr.h"
#include "func_args.h"

struct TypeBase
{
	/* gives sym we're sitting on  */
	const Type		*tb_type;
	const SymbolTable	*tb_symtab;
	const SymbolTableEnt	*tb_lastsym;
	/* expressions that'll take us to sym */
	Expr			*tb_diskoff;
	Expr			*tb_parambuf;
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
	: func_args(NULL),
	  cur_scope(in_cur_scope)
	{
		assert (in_cur_scope != NULL);
	}

	/* evaluating for a function */
	EvalCtx(const FuncArgs* in_func_args)
	:	func_args(in_func_args),
		cur_scope(NULL)
	{
	}
		

	virtual ~EvalCtx() {}

	const SymbolTable*	getCurrentScope() const { return cur_scope; }
	const FuncArgs*		getFuncArgs(void) const { return func_args; }

	/* given an expression that is either a scalar id or an array id,
	 * return the name and (if applicable) the index into the type
	 */
	bool toName(const Expr* e, std::string& in_str, Expr* &idx) const;

	Expr* resolve(const Id* id) const;
	Expr* resolve(const IdArray* ida) const;
	Expr* resolve(const IdStruct* ids) const;

	const Type* getType(const Expr* e) const;
	const Type* getTypeId(const Id* id) const;
	const Type* getTypeIdStruct(const IdStruct* id) const;

private:
	EvalCtx(void);
	Expr* resolveArrayInType(const IdArray* ida) const;
	bool setNewOffsets(
		struct TypeBase& current_base, const Expr* idx) const;


protected:
	const FuncArgs*		func_args;
	const SymbolTable*	cur_scope;

	const SymbolTable*	symtabByName(const std::string& s) const;
	const Type*		typeByName(const std::string& s) const;


	bool resolveTail(
		TypeBase			&tb,	/* current base */
		const IdStruct* 		ids,
		IdStruct::const_iterator	ids_begin) const;

};

#endif
