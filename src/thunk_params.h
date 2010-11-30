#ifndef THUNKPARAMS_H
#define THUNKPARAMS_H

#include <llvm/Support/IRBuilder.h>
#include "thunk_fieldfunc.h"

#define THUNKPARAM_PREFIX	"__thunkparams_"
#define THUNKPARAM_EMPTYNAME	THUNKPARAM_PREFIX"empty"

/*
 * used to convert parent params into type params
 */
class ThunkParams : public ThunkFieldFunc
{
public:
	ThunkParams() : no_params(true), exprs(NULL) {}
	ThunkParams(ExprList* expr_list) 
	: no_params(false), exprs(expr_list) 
	{
		setPrefix("thunkparams");
	} 

	virtual ~ThunkParams();

	virtual FCall* copyFCall(void) const;
	FCall* copyFCall(unsigned int idx) const;
	virtual ThunkParams* copy(void) const;

	virtual bool genProto(void) const;
	virtual bool genCode(void) const;
	bool genCodeExprs(void) const;

	static ThunkParams* createNoParams();
	static ThunkParams* createCopyParams();
	static ThunkParams* createSTUB(void);

	static bool genProtoEmpty(void);
	static bool genProtoByName(const std::string& name);
	static bool genCodeEmpty(void);

protected:
	const std::string getFCallName(void) const;
private:
	bool storeIntoParamBuf(llvm::AllocaInst *params_out_ptr) const;
	bool		no_params;
	ExprList	*exprs;
};



#endif
