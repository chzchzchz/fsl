#ifndef WRITEPKT_H
#define WRITEPKT_H

#include <string>
#include <map>
#include <list>

#include "AST.h"
#include "collection.h"
#include "expr.h"
#include "code_builder.h"

typedef std::map<std::string, class WritePkt*>		writepkt_map;
typedef std::list<class WritePkt*>			writepkt_list;
typedef std::list<class WritePktBlk*>			wblk_list;

class WritePktBlk;
class WritePkt;

class WritePktStmt
{
public:
	virtual ~WritePktStmt() { if (cond_expr) delete cond_expr; }
	virtual void print(std::ostream& out) const = 0;
	void setParent(WritePktBlk* wpb, int n) { parent = wpb; stmt_num = n; }
	WritePktBlk* getParent(void) const { return parent; }
	int getBlkNum(void) const { assert (stmt_num >= 0); return stmt_num; }
	void genProto() const;
	virtual void genCode(void) const;
	std::string getFuncName(void) const;
	void setCond(CondExpr* ce) { cond_expr = ce; }
	const CondExpr* getCond(void) const { return cond_expr; }
protected:
	WritePktStmt(void) : parent(NULL), stmt_num(-1), cond_expr(NULL) { }
	WritePktBlk	*parent;
	int		stmt_num;
	CondExpr	*cond_expr;
};

class WritePktStruct : public WritePktStmt
{
public:
	WritePktStruct(IdStruct* in_ids, Expr* in_e)
	 : e(in_e), ids(in_ids) { assert (ids != NULL); }
	virtual ~WritePktStruct() { delete ids; delete e; }
	virtual void print(std::ostream& out) const;
	virtual void genCode(void) const;
private:
	Expr		*e;
	IdStruct	*ids;
};

class WritePktId : public WritePktStmt
{
public:
	WritePktId(Id* in_id, Expr* in_e)
	: e(in_e), id(in_id) { assert (id != NULL); }
	virtual ~WritePktId() { delete id; delete e;}
	virtual void print(std::ostream& out) const;
private:
	Expr*	e;
	Id*	id;
};

class WritePktCall : public WritePktStmt
{
public:
	WritePktCall(Id* in_name, ExprList* in_exprs)
	: name(in_name), exprs(in_exprs) {}
	virtual ~WritePktCall(void) { delete name; delete exprs; }
	virtual void print(std::ostream& out) const;
private:
	Id*		name;
	ExprList*	exprs;
};

class WritePktArray : public WritePktStmt
{
public:
	WritePktArray(IdArray* in_a, Expr* in_e)
	: e(in_e), a(in_a) { assert (a != NULL); }
	virtual ~WritePktArray() { delete a; delete e;}
	virtual void print(std::ostream& out) const;
private:
	Expr	*e;
	IdArray	*a;
};

class WritePktBlk : public PtrList<WritePktStmt>
{
public:
	WritePktBlk() : parent(NULL), blk_num(-1) {}
	virtual ~WritePktBlk() {}
	virtual void print(std::ostream& out) const;
	void setParent(WritePkt* wp, unsigned int);
	WritePkt* getParent(void) const { return parent; }
	int getBlkNum(void) const { assert(blk_num >= 0); return blk_num; }

private:
	WritePkt	*parent;
	int		blk_num;
};

class WritePkt : public GlobalStmt, PtrList<WritePktBlk>
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

	void generateParamThunk(
		std::string& protoname, class EvalCtx* ectx,
		ExprList* exprs) const;

	void genLoadArgs(llvm::Function* );

	const VarScope* getVarScope(void) const { return &vscope; }
private:
	void genProtoParameters(void) const;
	void genStmtFuncProtos(void) const;
	void genStmtFuncCode(void) const;

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
	virtual void print(std::ostream& out) const;
private:
	WritePktBlk	*wpb;
};

#endif
