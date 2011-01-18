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

	Expr*	getLocal(Expr* closure, Expr* disk_bit_offset, Expr* num_bits);
	Expr*	getLocalArray(
		Expr* clo,
		Expr* idx, Expr* bits_in_type, Expr* base_offset, Expr* bits_in_array);

	Expr*	writeVal(const Expr* loc, const Expr* sz, const Expr* val);

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
	const std::string getMemoTabName(void) const;

	Expr	*toPhys(const Expr* clo, const Expr* off);

	Expr	*maxValue(ExprList* exprs);
	Expr	*computeArrayBits(const class ThunkField* tf, const Expr* idx);
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
