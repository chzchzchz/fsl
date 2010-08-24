#ifndef RUNTIME_INTERFACE_H
#define RUNTIME_INTERFACE_H

#include <string>
#include "expr.h"

class RTInterface
{
public:
	RTInterface() {}
	virtual ~RTInterface() {}

	void	loadRunTimeFuncs(class CodeBuilder* cb);

	Expr*	getLocal(Expr* disk_bit_offset, Expr* num_bits);
	Expr*	getLocalArray(
		Expr* idx, Expr* bits_in_type, Expr* base_offset, Expr* bits_in_array);
	Expr*	getDynOffset(const class Type* user_type);
	Expr*	getDynParams(const class Type* user_type);

	Expr*	getThunkArgOffset(void);
	Expr*	getThunkArgParamPtr(void);


	const std::string getThunkArgOffsetName(void);
	const std::string getThunkArgParamPtrName(void);

	Expr	*maxValue(ExprList* exprs);
	Expr	*computeArrayBits(const class ThunkField* tf);
	Expr	*fail(void);
private:

};

#endif
