#ifndef SYMTAB_THUNKBUILDER_H
#define SYMTAB_THUNKBUILDER_H

#include <list>
#include "collection.h"
#include "symtab.h"

class SymTabThunkBuilder : public TypeVisitAll
{
public:
	SymTabThunkBuilder() 
	: cur_symtab(NULL),
	  cur_type(NULL),
	  weak_c(0),
	  last_tf(NULL) {}

	virtual ~SymTabThunkBuilder() { assert (last_tf == NULL); }
	SymbolTable* getSymTab(
		const Type* t, 
		std::list<SymbolTable*>& union_symtabs);
	SymbolTable* getSymTab(const TypeUnion* tu);
	virtual void visit(const TypeDecl* td);
	virtual void visit(const TypeUnion* tu);
	virtual void visit(const TypeParamDecl*);
	virtual void visit(const TypeCond* tb);
	virtual void visit(const TypeFunc* tf);

private:
	void setLastThunk(ThunkField* last_tf);
	void addUnionToSymTab(
		const std::string&	field_name,
		const IdArray*		array);
	void addToCurrentSymTab(
		const std::string& 	type_str,
		const std::string&	field_name,
		const IdArray*		array);

	void addToCurrentSymTab(
		const std::string&	type_str,
		const std::string&	field_name,
		ThunkField* 		tfield);

	CondExpr* getConds(void) const;

	Expr* copyFromBase(void) const;
	Expr* copyCurrentOffset(void) const;

	const Type* getTypeFromUnionSyms(const std::string& field_name) const;
	void rebaseByCond(
		const Expr* 		initial_off,
		const CondExpr*		cond_expr,
		const ThunkField*	last_tf_true,
		const ThunkField*	last_tf_false);
	
	ThunkField* alignBits(const TypeFunc* tf);
	ThunkField* alignBytes(const TypeFunc* tf);
	ThunkField* skipBits(const TypeFunc* tf);
	ThunkField* skipBytes(const TypeFunc* tf);
	ThunkField* setBits(const TypeFunc* tf);
	ThunkField* setBytes(const TypeFunc* tf);


	SymbolTable		*cur_symtab;
	ThunkType		*cur_thunk_type;

	ThunkField		*last_tf;
	Expr			*last_tf_union_off;
	PtrList<ThunkField>	union_tf_list;
	std::list<SymbolTable*>	union_symtabs;
	cond_list		cond_stack;

	const Type		*cur_type;
	unsigned int		union_c;
	unsigned int		weak_c;
	unsigned int		field_count;
};

#endif
