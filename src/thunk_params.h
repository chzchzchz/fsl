#ifndef THUNKPARAMS_H
#define THUNKPARAMS_H

#include "thunk_fieldfunc.h"

#define THUNKPARAM_PREFIX	"__thunkparams_"
#define THUNKPARAM_EMPTYNAME	THUNKPARAM_PREFIX"empty"
/*
 * used to convert parent params into type params
 */
class ThunkParams : public ThunkFieldFunc
{
public:
	ThunkParams() : no_params(true) {}

	virtual ~ThunkParams() {}

	virtual FCall* copyFCall(void) const;
	virtual ThunkParams* copy(void) const;

	virtual bool genProto(void) const;
	virtual bool genCode(void) const;

	static ThunkParams* createNoParams();
	static ThunkParams* createCopyParams();
	static ThunkParams* createSTUB(void);

	static bool genProtoEmpty(void);
	static bool genProtoByName(const std::string& name);
	static bool genCodeEmpty(void);

protected:
	const std::string getFCallName(void) const;
private:
	bool		no_params;
};


#endif
