#ifndef ASSERTS_H
#define ASSERTS_H

#include "collection.h"

typedef PtrList<class Assertion>		assertion_list;
typedef std::map<std::string, class Asserts*>	assert_map;
typedef std::list<class Asserts*>		assert_list;

class Asserts
{
public:
	Asserts(const Type* t);
	virtual ~Asserts(void) {}

	const assertion_list* getAsserts(void) const 
	{ 
		return &assert_elems; 
	}

/* not yet... */
#if 0
	const assertrange_list* getAssertsRange(void) const
	{
		return &points_range_elems;
	}
#endif
	const Type* getType(void) const { return src_type; }

	unsigned int getNumAsserts(void) const
	{
		return	assert_elems.size();
	}

	void genCode(void);
	void genProtos(void);

private:
	void loadAsserts(void);

	assertion_list		assert_elems;
	const Type*		src_type;
	unsigned int		seq;

};

class Assertion
{
public:
	Assertion(
		const Type*	in_src_type,
		const CondExpr	*in_pred,
		unsigned int	in_seq);
	virtual ~Assertion(void);

	void genCode(void);
	void genProtos(void);

	const std::string getFCallName(void) const;
private:
	Assertion() {}
	const Type*	src_type;
	CondExpr*	pred;
	unsigned int	seq;
};


#endif