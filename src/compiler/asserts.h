#ifndef ASSERTS_H
#define ASSERTS_H

#include "collection.h"
#include "table_gen.h"
#include "annotation.h"

typedef PtrList<class Assertion>		assertion_list;
typedef std::map<std::string, class Asserts*>	assert_map;
typedef std::list<class Asserts*>		assert_list;

class Asserts : public Annotation, public TableWriter
{
public:
	Asserts(const Type* t);
	virtual ~Asserts(void);

	unsigned int getNumAsserts(void) const { return assert_elems.size(); }
	virtual const anno_ent_list* getEntryList(void) const
		{ return (const anno_ent_list*)(&assert_elems); }

	virtual void genExterns(TableGen* tg);
	virtual void genTables(TableGen* tg);
protected:
	virtual void load(const Preamble* p);
private:
	assertion_list	assert_elems;
};

class Assertion : public AnnotationEntry
{
public:
	Assertion(
		Asserts*	in_parent,
		const Preamble* in_pre,
		const CondExpr* in_pred);
	virtual ~Assertion(void);

	void genCode(void);
	void genProto(void);
	void genInstance(TableGen* tg) const;

private:
	Assertion() {}
	CondExpr*	pred;
};

#endif