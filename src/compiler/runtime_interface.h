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

	Expr	*getDebugCall(Expr* pass_val);
	Expr	*getDebugCallTypeInstance(const Type* clo_type, Expr* clo_expr);

#define RT_GET_EXPR(x)	Expr* get##x(void) \
	{ return new Id(get##x##Name()); }
	RT_GET_EXPR(ThunkClosure)
	RT_GET_EXPR(ThunkArgOffset)
	RT_GET_EXPR(ThunkArgParamPtr)
	RT_GET_EXPR(ThunkArgVirt)
	RT_GET_EXPR(ThunkArgIdx)
#define RT_GET_NAME(x,y) const std::string get##x##Name(void) { return y; }
	RT_GET_NAME(ThunkArgIdx, "__thunkparamsf_idx")
	RT_GET_NAME(ThunkClosure, "__thunk_closure")
	RT_GET_NAME(ThunkArgVirt, "__thunk_virt")
	RT_GET_NAME(ThunkArgOffset, "__thunk_arg_off")
	RT_GET_NAME(ThunkArgParamPtr, "__thunk_arg_params")
	RT_GET_NAME(MemoTab, "__fsl_memotab")

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
		const Expr* clo,
		/* nth element to find */
		const Expr* num_elems);
	Expr	*fail(void);
private:
};

#endif
