#ifndef ASSERTS_H
#define ASSERTS_H

#include "collection.h"
#include "table_gen.h"

typedef PtrList<class Assertion>		assertion_list;
typedef std::map<std::string, class Asserts*>	assert_map;
typedef std::list<class Asserts*>		assert_list;

class Asserts : public TableWriter
{
public:
	Asserts(const Type* t);
	virtual ~Asserts(void) {}

	const assertion_list* getAsserts(void) const { 	return &assert_elems; }
	const Type* getType(void) const { return src_type; }
	unsigned int getNumAsserts(void) const { return assert_elems.size(); }

	void genCode(void);
	void genProtos(void);

	virtual void genExterns(TableGen* tg);
	virtual void genTables(TableGen* tg);
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
		const Id	*in_name,
		unsigned int	in_seq);
	virtual ~Assertion(void);

	void genCode(void);
	void genProtos(void);
	void genInstance(TableGen* tg) const;
	const std::string getFCallName(void) const;
	const Id* getName(void) const { return name; }
private:
	Assertion() {}
	const Type*	src_type;
	CondExpr*	pred;
	const Id*	name;
	unsigned int	seq;
};

#endif