#ifndef FUNC_H
#define FUNC_H

#include <string>
#include <map>
#include <list>

#include "AST.h"
#include "collection.h"
#include "expr.h"
#include "code_builder.h"

extern int yylineno;

typedef std::map<std::string, class Func*>		func_map;
typedef std::list<class Func*>				func_list;

class FuncStmt 
{
public:
	virtual ~FuncStmt() {}
	unsigned int getLineNo() const { return lineno; }
	virtual llvm::Value* codeGen(const class EvalCtx*) const = 0;
	virtual void setOwner(class FuncBlock* o) 
		{ assert (owner == NULL); owner = o; }
	class FuncBlock* getOwner(void) const { return owner; }
	class Func* getFunc(void) const;
	class llvm::Function* getFunction() const;

	void setFunc(class Func* f) { assert (f_owner == NULL); f_owner = f; }
protected:
	FuncStmt() : owner(NULL), f_owner(NULL) { lineno = yylineno; }
private:
	class FuncBlock*	owner;
	class Func*		f_owner;
	unsigned int 		lineno;
};

class FuncBlock : public FuncStmt, public PtrList<FuncStmt>
{
public:
	FuncBlock() {}
	virtual ~FuncBlock() {}
	llvm::Value* codeGen(const class EvalCtx*) const;
	virtual void add(FuncStmt* fs) 
	{
		fs->setOwner(this);
		PtrList<FuncStmt>::add(fs);
	}

	llvm::AllocaInst* getVar(const std::string& s) const;
	bool addVar(const std::string& name, llvm::AllocaInst* ai);

private:
	llvm_var_map	vars;
};

class FuncCondStmt : public FuncStmt
{
public:
	FuncCondStmt(CondExpr* ce, FuncStmt* taken, FuncStmt* not_taken)
	: cond(ce), is_true(taken), is_false(not_taken)
	{
		assert (cond != NULL);
		assert (is_true != NULL);
	}

	virtual ~FuncCondStmt() 
	{
		delete cond;
		delete is_true;
		if (is_false != NULL) delete is_false;
	}

	virtual void setOwner(class FuncBlock* o) 
	{
		FuncStmt::setOwner(o);
		is_true->setOwner(o);
		if (is_false != NULL) is_false->setOwner(o);
	}

	llvm::Value* codeGen(const class EvalCtx*) const;
private:
	CondExpr	*cond;
	FuncStmt	*is_true;
	FuncStmt	*is_false;
};

class FuncRet : public FuncStmt
{
public:
	FuncRet(Expr* ret_expr) : expr(ret_expr) { assert (expr != NULL); }
	virtual ~FuncRet() { delete expr; }

	llvm::Value* codeGen(const class EvalCtx*) const;
private:
	Expr	*expr;
};

class FuncAssign : public FuncStmt
{
public:
	FuncAssign(Id* in_scalar, Expr* in_expr) 
	: scalar(in_scalar), array(NULL), expr(in_expr)
	{
		assert (scalar != NULL);
		assert (expr != NULL);
	}

	FuncAssign(IdArray* in_array, Expr* in_expr) 
	: scalar(NULL), array(in_array), expr(in_expr)
	{
		assert (array != NULL);
		assert (expr != NULL);
	}

	virtual ~FuncAssign()
	{
		if (scalar != NULL) delete scalar;
		if (array != NULL) delete array;
		delete expr;
	}

	llvm::Value* codeGen(const class EvalCtx*) const;
private:
	Id	*scalar;
	IdArray	*array;
	Expr	*expr;
};

class FuncDecl : public FuncStmt
{ 
public:
	FuncDecl(Id* in_type, Id* in_name) 
	: type(in_type), scalar(in_name), array(NULL)
	{
		assert (type != NULL);
		assert (scalar != NULL);
	}
	FuncDecl(Id* in_type, IdArray* in_name) 
	: type(in_type), scalar(NULL), array(in_name)
	{
		assert (type != NULL);
		assert (array != NULL);
	} 

	virtual ~FuncDecl()
	{
		delete type;
		if (scalar != NULL) delete scalar;
		if (array != NULL) delete array;
	}

	llvm::Value* codeGen(const class EvalCtx*) const;
private:
	Id	*type;
	Id	*scalar;
	IdArray	*array;
};

class Func : public GlobalStmt 
{
public:
	Func(Id* in_ret_type, Id* in_name, ArgsList* in_al, FuncBlock* in_block)
	: 	ret_type(in_ret_type),
		name(in_name),
		args(in_al),
		block(in_block)
	{
		assert (ret_type != NULL);
		assert (name != NULL);
		assert (args != NULL);
		assert (block != NULL);
		block->setFunc(this);
	}

	virtual ~Func() 
	{
		delete ret_type;
		delete name;
		delete args;
		delete block;
	}

	void print(std::ostream& out) const { out << "Func"; }

	const std::string& getName() const { return name->getName(); }
	const std::string& getRet() const { return ret_type->getName(); }
	llvm::Function* getFunction(void) const;

	const ArgsList* getArgs() const { return args; }
	llvm::Value* codeGen(const class EvalCtx*) const;
private:
	Id		*ret_type;
	Id		*name;
	ArgsList	*args;
	FuncBlock	*block;
};

void gen_func_code(Func* f);
void gen_func_proto(const Func* f);

#endif
