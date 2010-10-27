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
	Expr*	getDynVirt(const class Type* user_type);

	Expr*	getThunkClosure(void);
	Expr*	getThunkArgOffset(void);
	Expr*	getThunkArgParamPtr(void);
	Expr*	getThunkArgVirt(void);
	Expr*	getThunkArgIdx(void);

	Expr	*getDebugCall(Expr* pass_val);

	Expr	*getDebugCallTypeInstance(const Type* clo_type, Expr* clo_expr);

	const std::string getThunkClosureName(void);
	const std::string getThunkArgOffsetName(void);
	const std::string getThunkArgParamPtrName(void);
	const std::string getThunkArgVirtName(void);
	const std::string getThunkArgIdxName(void);


	Expr	*maxValue(ExprList* exprs);
	Expr	*computeArrayBits(const class ThunkField* tf);
	Expr	*computeArrayBits(
		/* type of the parent*/
		const Type* t,
		/* fieldall idx for type */
		unsigned int fieldall_num,
		/* converted into single closure */
		const Expr* diskoff, 	/* base */
		const Expr* params,	/* params */
		const Expr* virt,	/* virt */
		/* nth element to find */
		const Expr* num_elems);
	Expr	*fail(void);
private:


};

#endif
