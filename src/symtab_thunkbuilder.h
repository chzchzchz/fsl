#ifndef SYMTAB_THUNKBUILDER_H
#define SYMTAB_THUNKBUILDER_H

#include "symtab.h"

class SymTabThunkBuilder : public TypeVisitAll
{
public:
	SymTabThunkBuilder(const ptype_map& ptypes) 
	: cur_symtab(NULL),
	  cur_type(NULL),
	  tm(ptypes),
	  weak_c(0),
	  ret_pt(NULL) {}

	virtual ~SymTabThunkBuilder() {}
	SymbolTable* getSymTab(const Type*, PhysicalType * &ptt);
	SymbolTable* getSymTab(const TypeBlock*, PhysicalType * &ptt);
	virtual void visit(const TypeDecl* td);
	virtual void visit(const TypeUnion* tu);
	virtual void visit(const TypeParamDecl*);
	virtual void visit(const TypeBlock* tb);
	virtual void visit(const TypeCond* tb);

private:
	void retType(PhysicalType* pt);
	void addToCurrentSymTabAnonType(
		const PhysicalType*	pt,
		const std::string&	field_name,
		const Id*		scalar,
		const IdArray*		array);

	void addToCurrentSymTab(
		const std::string& 	type_str,
		const std::string&	field_name,
		const Id*		scalar,
		const IdArray*		array,
		ExprList*		type_args);

	SymbolTable		*cur_symtab;
	const Type		*cur_type;
	const ptype_map		&tm;
	unsigned int		weak_c;
	PhysicalType		*ret_pt;
};

#endif
