#ifndef FUNCEVALCTX_H
#define FUNCEVALCTX_H

#include "evalctx.h"

/**
 * overloads teh standard evalctx so that we can rewrite structures using 
 * thunkoffs-- this is necessary because if we used the expressions stored
 * in the symtab, we'd run into some nasty scoping issues...
 */
class FuncEvalCtx : public EvalCtx
{
public:
	FuncEvalCtx(	
		const SymbolTable&	in_cur_scope,
		const symtab_map&	in_all_types,
		const const_map&	in_constants)
	: EvalCtx(in_cur_scope, in_all_types, in_constants) {}

	virtual ~FuncEvalCtx() {}
private:
	FuncEvalCtx();

protected:
	virtual Expr* getStructExpr(
		const Expr			*base,
		const SymbolTable		*first_symtab,
		const IdStruct::const_iterator	ids_first,
		const IdStruct::const_iterator	ids_end,
		const PhysicalType*		&final_type) const;

};

#endif
