#ifndef RUNTIME_INTERFACE_H
#define RUNTIME_INTERFACE_H

#include <string>
#include "expr.h"

class RTInterface
{
public:
	RTInterface() {}
	virtual ~RTInterface() {}

	Expr*	getLocal(Expr* disk_bit_offset, Expr* num_bits);
	Expr*	getLocalArray(
		Expr* idx, Expr* bits_in_type, Expr* base_offset, Expr* bits_in_array);
	Expr*	getDyn(const class Type* user_type);
	Expr*	getThunkArg(void);
	const std::string getThunkArgName(void);
	Expr	*maxValue(ExprList* exprs);
	Expr	*fail(void);
private:

};

#endif
