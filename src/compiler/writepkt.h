#ifndef WRITEPKT_H
#define WRITEPKT_H

#include <string>
#include <map>
#include <list>
#include "type.h"
#include "varscope.h"
#include "AST.h"
#include "collection.h"
#include "expr.h"
#include "table_gen.h"
#include "code_builder.h"

typedef std::map<std::string, class WritePkt*>		writepkt_map;
typedef std::list<class WritePkt*>			writepkt_list;
typedef std::list<class WritePktBlk*>			wblk_list;

class WritePktBlk;
class WritePkt;
class WritePktInstance;

class WritePktStmt
{
public:
	virtual ~WritePktStmt() { if (cond_expr) delete cond_expr; }
	virtual std::ostream& print(std::ostream& out) const = 0;
	void setParent(WritePktBlk* wpb, int n) { parent = wpb; stmt_num = n; }
	WritePktBlk* getParent(void) const { return parent; }
	int getBlkNum(void) const { assert (stmt_num >= 0); return stmt_num; }
	virtual void genProto() const { assert (0 == 1 && "BASE FUNC"); }
	virtual void genCode(void) const;
	virtual std::string getFuncName(void) const;
	virtual void printExterns(class TableGen* tg) const;
	void setCond(CondExpr* ce) { cond_expr = ce; }
	const CondExpr* getCond(void) const { return cond_expr; }
	bool genCodeHeader(
		llvm::Function* f = NULL,
		llvm::BasicBlock* bb_entry = NULL,
		llvm::BasicBlock* bb_then = NULL,
		llvm::BasicBlock* bb_else = NULL) const;
protected:
	WritePktStmt(void) : parent(NULL), stmt_num(-1), cond_expr(NULL) { }
	WritePktBlk	*parent;
	int		stmt_num;
	CondExpr	*cond_expr;
};

class WritePktId : public WritePktStmt
{
public:
	WritePktId(Id* in_id, Expr* in_e)
	: e(in_e), id(in_id) { assert (id != NULL); }
	virtual ~WritePktId() { delete id; delete e;}
	virtual std::ostream& print(std::ostream& out) const;
private:
	Expr*	e;
	Id*	id;
};

class WritePktArray : public WritePktStmt
{
public:
	WritePktArray(IdArray* in_a, Expr* in_e)
	: e(in_e), a(in_a) { assert (a != NULL); }
	virtual ~WritePktArray() { delete a; delete e;}
	virtual std::ostream& print(std::ostream& out) const;
private:
	Expr	*e;
	IdArray	*a;
};

class WritePktBlk : public PtrList<WritePktStmt>
{
public:
	WritePktBlk() : parent(NULL), blk_num(-1) {}
	virtual ~WritePktBlk() {}
	virtual std::ostream& print(std::ostream& out) const;
	void setParent(WritePkt* wp, unsigned int);
	WritePkt* getParent(void) const { return parent; }
	int getBlkNum(void) const { assert(blk_num >= 0); return blk_num; }
	unsigned int getNumFuncs(void) const;
	unsigned int getNumCalls(void) const;
private:
	WritePkt	*parent;
	int		blk_num;
};

class WritePkt : public GlobalStmt, public PtrList<WritePktBlk>
{
public:
	WritePkt(
		Id* in_name, ArgsList* in_args,
		const wblk_list& wbs);

	void genCode(void) const;
	void genProtos(void) const;
	std::string getName(void) const { return name->getName(); }
	const ArgsList* getArgs(void) const { return args; }

	virtual ~WritePkt()
	{
		delete name;
		delete args;
	}

	WritePktInstance* getInstance(
		const std::string& protoname,
		const class Type* clo_type,
		const ExprList* exprs) const;

	void genExterns(class TableGen* tg) const;
	void genTables(class TableGen* tg) const;

	void genLoadArgs(llvm::Function* );

	const VarScope* getVarScope(void) const { return &vscope; }

	static WritePktInstance* getInstance(
		const Expr* wpkt_call,
		const std::string& protoname,
		const class Type* clo_type);
private:
	void genStmtFuncProtos(void) const;
	void genStmtFuncCode(void) const;
	void genFuncTables(TableGen* tg) const;
	void genCallsTables(TableGen* tg) const;
	void genWpktStructs(TableGen* tg) const;

	WritePkt() {}
	Id		*name;
	ArgsList	*args;
	VarScope	vscope;
};

class WritePktAnon : public WritePktStmt
{
public:
	WritePktAnon(WritePktBlk* in_wpb) : wpb(in_wpb) {}
	virtual ~WritePktAnon(void) { delete wpb; }
	virtual std::ostream& print(std::ostream& out) const;
private:
	WritePktBlk	*wpb;
};

#endif
