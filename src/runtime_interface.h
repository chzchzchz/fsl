#ifndef RUNTIME_INTERFACE_H
#define RUNTIME_INTERFACE_H

#include <string>
#include "expr.h"

#define RT_CLO_IDX_OFFSET			0
#define RT_CLO_IDX_PARAMS			1
#define RT_CLO_IDX_XLATE			2

class RTInterface
{
public:
	RTInterface() {}
	virtual ~RTInterface() {}

	void	loadRunTimeFuncs(class CodeBuilder* cb);

	Expr*	getLocal(Expr* closure, Expr* disk_bit_offset, Expr* num_bits);
	Expr*	getLocalArray(
		Expr* clo,
		Expr* idx, Expr* bits_in_type, Expr* base_offset, Expr* bits_in_array);
	Expr*	getDynOffset(const class Type* user_type);
	Expr*	getDynParams(const class Type* user_type);
	Expr*	getDynClosure(const class Type* user_type);

	Expr*	getThunkClosure(void);
	Expr*	getThunkArgOffset(void);
	Expr*	getThunkArgParamPtr(void);

	Expr	*getDebugCall(Expr* pass_val);

	Expr	*getDebugCallTypeInstance(const Type* clo_type, Expr* clo_expr);

	const std::string getThunkClosureName(void);
	const std::string getThunkArgOffsetName(void);
	const std::string getThunkArgParamPtrName(void);

	Expr	*maxValue(ExprList* exprs);
	Expr	*computeArrayBits(const class ThunkField* tf);
	Expr	*computeArrayBits(
		const Type* t,
		const Expr* diskoff, const Expr* params,
		const Expr* idx);
	Expr	*fail(void);
private:

};

#endif
