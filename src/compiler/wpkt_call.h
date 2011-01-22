#ifndef WRITEPKTCALL_H
#define WRITEPKTCALL_H
#include "writepkt.h"

class WritePktCall : public WritePktStmt
{
public:
	WritePktCall(Id* in_name, ExprList* in_exprs);
	virtual ~WritePktCall(void) { delete name; delete exprs; }
	virtual std::ostream& print(std::ostream& out) const;
	virtual void genCode(void) const;
	virtual std::string getFuncName(void) const;
	virtual void genProto(void) const;
	virtual void printExterns(class TableGen* tg) const;
	void genTableInstance(class TableGen* tg) const;
private:
	std::string getCondFuncName(void) const;
	void genCodeWpkt2Wpkt(void) const;
	void genCodeCond(void) const;
	Id*		name;
	ExprList*	exprs;
};

#endif
