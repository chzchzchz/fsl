#ifndef EVALCTX_H
#define EVALCTX_H

#include "symtab.h"
#include "phys_type.h"
#include "type.h"
#include "expr.h"

/**
 * will eventualy want to replace this with different eval contexts so
 * that we can use it for handling local scopes within functions
 */
class EvalCtx
{
public:
	EvalCtx(
		const SymbolTable&	in_cur_scope,
		const symtab_map&	in_all_types,
		const const_map&	in_constants)
	: cur_scope(in_cur_scope),
	  all_types(in_all_types),
	  constants(in_constants)
	{
	}

	virtual ~EvalCtx() {}

	const SymbolTable&	getCurrentScope() const { return cur_scope; }
	const symtab_map&	getGlobalScope() const { return all_types; }
	const const_map&	getConstants() const { return constants; }


	/* given an expression that is either a scalar id or an array id,
	 * return the name and (if applicable) the index into the type
	 */
	bool toName(const Expr* e, std::string& in_str, Expr* &idx) const;

	Expr* resolve(const Id* id) const;
	Expr* resolve(const IdArray* ida) const;
	Expr* resolve(const IdStruct* ids) const;

private:
	EvalCtx(void);

protected:

	/* do we need to know incoming from_base? */
	const SymbolTable&	cur_scope;
	const symtab_map&	all_types;
	const const_map&	constants;

	const SymbolTable*	symtabByName(const std::string& s) const;
	const Type*		typeByName(const std::string& s) const;

	Expr* resolveCurrentScope(const IdStruct* ids) const;
	Expr* resolveGlobalScope(const IdStruct* ids) const;
	Expr* resolveFuncArg(const IdStruct* ids) const;

	Expr* getStructExpr(
		const Expr			*base,
		const SymbolTable		*first_symtab,
		const IdStruct::const_iterator	ids_first,
		const IdStruct::const_iterator	ids_end,
		const PhysicalType*		&final_type) const;

	virtual Expr* buildTail(
		IdStruct::const_iterator	it,
		IdStruct::const_iterator	ids_end,
		Expr*				ret,
		const SymbolTable*		parent_symtab,
		const PhysicalType*		&final_type) const;


	Expr* getStructExprBase(
		const SymbolTable		*first_symtab,
		const IdStruct::const_iterator	it_begin,
		const SymbolTable*		&next_symtab) const;
};

#endif
