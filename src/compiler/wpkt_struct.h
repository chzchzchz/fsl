#ifndef WPKTSTRUCT_H
#define WPKTSTRUCT_H

#include "writepkt.h"

class WritePktStruct : public WritePktStmt
{
public:
	WritePktStruct(IdStruct* in_ids, Expr* in_e)
	 : e(in_e), ids(in_ids) { assert (ids != NULL); }
	virtual ~WritePktStruct() { delete ids; delete e; }
	virtual std::ostream& print(std::ostream& out) const;
	virtual void printExterns(class TableGen* tg) const;
	virtual void genCode(void) const;
	virtual void genProto(void) const;
private:
	Expr		*e;
	IdStruct	*ids;
};

#endif
