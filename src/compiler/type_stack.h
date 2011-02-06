#ifndef TYPESTACK_H
#define TYPESTACK_H

#include "symtab.h"
#include "expr.h"
#include "type.h"

#include <stack>

/* Anthony, replace this with a 'type stack', instead of the shit we're using now */
/* Main idea; keep stack of resolved ID/FCalls... */
class TypeBase
{
public:
	TypeBase(
		const Type* in_tb_type,
		const SymbolTable* in_tb_symtab,
		const SymbolTableEnt* in_tb_lastsym,
		Expr* in_tb_diskoff,
		Expr* in_tb_parambuf,
		Expr* in_tb_virt);
	virtual ~TypeBase();

	bool setNewOffsets(
		const class EvalCtx* ectx,
		const Type* parent_type,
		const Expr* idx = NULL);
	Expr* setNewOffsetsArray(
		const class EvalCtx* ectx,
		const Type* parent_type,
		Expr* new_base,
		Expr** new_params,
		const ThunkField* tf,
		const Expr* idx) const;

	Expr* replaceClosure(Expr* in_expr) const;

	const Type* getType(void) const { return tb_type; }
	const SymbolTable* getSymTab(void) const { return tb_symtab; }
	const Expr* getDiskOff() const { return tb_diskoff; }
	const Expr* getParamBuf() const { return tb_parambuf; }
	const Expr* getVirt() const { return tb_virt; }
	const SymbolTableEnt* getSym(void) const { return tb_lastsym; }

private:
	/* gives sym we're sitting on  (e.g. the cursor) */
	const class Type	*tb_type;
	const SymbolTable	*tb_symtab;
	const SymbolTableEnt	*tb_lastsym;
	/* expressions that'll take us to sym on disk (e.g. the cursor) */
	Expr			*tb_diskoff;
	Expr			*tb_parambuf;
	Expr			*tb_virt;
};

typedef std::stack<TypeBase*> tb_stack;

class TypeStack
{
public:
	TypeStack(TypeBase* tb);
	virtual ~TypeStack();
	bool followIdIdx(const EvalCtx* ectx, std::string& id, Expr* idx);
	unsigned int size() const { return s.size(); }
	const TypeBase* getTop() const { return s.top(); }
	TypeBase* pop() { TypeBase* ret = s.top(); s.pop(); return ret; }
private:
	tb_stack	s;
protected:
	TypeStack() {}
};

#endif
