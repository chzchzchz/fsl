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
	  last_tf(NULL),
	  cur_type(NULL),
	  weak_c(0) {}

	virtual ~SymTabThunkBuilder() { assert (last_tf == NULL); }
	SymbolTable* getSymTab(
		const Type* t,
		std::list<SymbolTable*>& union_symtabs);
	SymbolTable* getSymTab(
		const ThunkType* parent_tt,
		const TypeUnion* tu);
	virtual void visit(const TypeDecl* td);
	virtual void visit(const TypeUnion* tu);
	virtual void visit(const TypeParamDecl*);
	virtual void visit(const TypeCond* tb);
	virtual void visit(const TypeFunc* tf);

	ThunkType* getCurThunkType(void) const { return cur_thunk_type; }
	unsigned int getCondDepth(void) const { return cond_stack.size(); }
	unsigned int getFieldCount(void) const { return field_count; }
	Expr* copyFromBase(void) const;
	Expr* copyCurrentOffset(void) const;
	void addToCurrentSymTab(
		const std::string&	type_str,
		const std::string&	field_name,
		ThunkField* 		tfield);

private:
	void setLastThunk(ThunkField* last_tf);
	void addUnionToSymTab(
		const std::string&	field_name,
		const IdArray*		array);
	void addToCurrentSymTab(
		const std::string& 	type_str,
		const std::string&	field_name,
		const IdArray*		array,
		bool			no_follow,
		const ExprList*		exprs = NULL);

	CondExpr* getConds(void) const;

	const Type* getTypeFromUnionSyms(const std::string& field_name) const;
	void rebaseByCond(
		const Expr* 		initial_off,
		const CondExpr*		cond_expr,
		const ThunkField*	last_tf_true,
		const ThunkField*	last_tf_false);

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

class ThunkBuilderFunc
{
public:
	virtual ~ThunkBuilderFunc() {}
	ThunkField* apply(const SymTabThunkBuilder*, const TypeFunc*) const;
protected:
	ExprList* setupArgs(
		const SymTabThunkBuilder* sb, const TypeFunc* tf) const;
	virtual ThunkField* doIt(
		const SymTabThunkBuilder* sb, const ExprList* args) const = 0;
	std::string getFuncName(const SymTabThunkBuilder*) const;
	ThunkBuilderFunc(const char* pf) : num_args(0), func_prefix(pf) {}
	unsigned int	num_args;
	const char	*func_prefix;
};

#define TBF_DECL(name, arg_c, fname)			\
class TBF##name : public ThunkBuilderFunc		\
{							\
public: TBF##name() : ThunkBuilderFunc(fname) { num_args = arg_c; }	\
	virtual ~TBF##name() {}				\
protected: virtual ThunkField* doIt(			\
	const SymTabThunkBuilder* sb, const ExprList* args) const;	\
}

TBF_DECL(SetBytes, 1, "set_bytes");
TBF_DECL(SetBits, 1, "set_bits");
TBF_DECL(SkipBytes, 1, "skip_bytes");
TBF_DECL(SkipBits, 1, "skip_bits");
TBF_DECL(AlignBits, 1, "align_bits");
TBF_DECL(AlignBytes, 1, "align_bytes");
TBF_DECL(PadPhysBytes, 2, "padphys_bytes");

void thunkbuilder_init_funcmap(void);

#endif
