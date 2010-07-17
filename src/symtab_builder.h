#ifndef SYMTAB_BUILDER_H
#define SYMTAB_BUILDER_H

#include "symtab.h"

class SymTabBuilder : public TypeVisitAll
{
public:
	SymTabBuilder(const ptype_map& ptypes) 
	: cur_symtab(NULL),
	  tm(ptypes),
	  off_thunked(NULL),
	  off_pure(NULL),
	  union_c(0),
	  ret_pt(NULL) {}

	virtual ~SymTabBuilder() {}
	SymbolTable* getSymTab(const Type*, PhysTypeUser* &ptt);
	virtual void visit(const TypeDecl* td);
	virtual void visit(const TypeUnion* tu);
	virtual void visit(const TypeParamDecl*);
	virtual void visit(const TypeBlock* tb);
	virtual void visit(const TypeCond* tb);
	virtual void visit(const TypeFunc* tf);

private:
	void updateOffset(Expr* e);
	void retType(PhysicalType* pt);

	SymbolTable		*cur_symtab;
	const Type		*cur_type;
	const ptype_map		&tm;
	Expr			*off_thunked;	/* uses thunk_off_arg */
	Expr			*off_pure;	/* does not use off_arg */
	unsigned int		union_c;
	PhysicalType		*ret_pt;
};

#endif
