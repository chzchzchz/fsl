#ifndef WRITEPKT_H
#define WRITEPKT_H

#include <string>
#include <map>
#include <list>

#include "AST.h"
#include "collection.h"
#include "expr.h"
#include "code_builder.h"

class WritePktBlk;

class WritePktStmt
{
public:
	virtual ~WritePktStmt();
	virtual void print(std::ostream& out) const = 0;
	void printParams(std::ostream& out) const;
protected:
	WritePktStmt(void) { }
	WritePktStmt(Expr* in_e) :
		e(in_e), wpb(NULL) { assert (e != NULL); }
	WritePktStmt(WritePktBlk* in_wpb) :
		e(NULL), wpb(in_wpb) { assert (wpb != NULL); }
	Expr		*e;
	WritePktBlk	*wpb;
};

class WritePktStruct : public WritePktStmt
{
public:
	WritePktStruct(IdStruct* in_ids, Expr* in_e)
	 : WritePktStmt(in_e), ids(in_ids) { assert (ids != NULL); }

	WritePktStruct(IdStruct* in_ids, WritePktBlk* in_wpb)
	 : WritePktStmt(in_wpb), ids(in_ids) { assert (ids != NULL); }
	virtual ~WritePktStruct() { delete ids; }
	virtual void print(std::ostream& out) const;
private:
	IdStruct	*ids;
};

class WritePktId : public WritePktStmt
{
public:
	WritePktId(Id* in_id, Expr* in_e)
	: WritePktStmt(in_e), id(in_id) { assert (id != NULL); }
	WritePktId(Id* in_id, WritePktBlk* in_wpb)
	: WritePktStmt(in_wpb), id(in_id) { assert (id != NULL); }
	virtual ~WritePktId() { delete id; }
	virtual void print(std::ostream& out) const;
private:
	Id*	id;
};

class WritePktArray : public WritePktStmt
{
public:
	WritePktArray(IdArray* in_a, Expr* in_e)
	: WritePktStmt(in_e), a(in_a) { assert (a != NULL); }
	WritePktArray(IdArray* in_a, WritePktBlk* in_wpb)
	: WritePktStmt(in_wpb), a(in_a) { assert (a != NULL); }
	virtual ~WritePktArray() { delete a; }
	virtual void print(std::ostream& out) const;
private:
	IdArray	*a;
};

class WritePktBlk : public PtrList<WritePktStmt>
{
public:
	WritePktBlk() {}
	virtual ~WritePktBlk() {}
	virtual void print(std::ostream& out) const;
private:
};

class WritePkt : public GlobalStmt
{
public:
	WritePkt(Id* in_name, ArgsList* in_args, WritePktBlk* in_wb)
		: name(in_name), args(in_args), wb(in_wb)
	{
		assert (name != NULL);
		assert (args != NULL);
		assert (wb != NULL);
	}

	virtual ~WritePkt()
	{
		delete name;
		delete args;
		delete wb;
	}
private:
	WritePkt() {}
	Id		*name;
	ArgsList	*args;
	WritePktBlk	*wb;
};

#endif
