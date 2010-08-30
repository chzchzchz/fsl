#include <assert.h>
#include "symtab.h"
#include "code_builder.h"
#include "util.h"

#include "asserts.h"

using namespace std;

typedef list<const Preamble*>	pre_assert_list;
extern CodeBuilder		*code_builder;
extern symtab_map		symtabs;


Asserts::Asserts(const Type* t)
: src_type(t), seq(0)
{
	assert (src_type != NULL);
	loadAsserts();
}


void Asserts::genCode(void)
{
	for (	assertion_list::const_iterator it = assert_elems.begin();
		it != assert_elems.end();
		it++)
	{	
		(*it)->genCode();
	}
}

void Asserts::genProtos(void)
{
	for (	assertion_list::const_iterator it = assert_elems.begin();
		it != assert_elems.end();
		it++)
	{	
		(*it)->genProtos();
	}
}

void Asserts::loadAsserts(void)
{
	pre_assert_list	pal;

	pal = src_type->getPreambles("assert");
	for (	pre_assert_list::const_iterator it = pal.begin();
		it != pal.end();
		it++)
	{
		const preamble_args	*args;
		const CondExpr		*predicate;

		args = (*it)->getArgsList();
		if (args == NULL || args->size() != 1) {
			cerr << "assert: expects one argument" << endl;
			continue;
		}

		predicate = (args->front())->getCondExpr();
		if (predicate == NULL) {
			cerr << "assert: expects conditional expression" << endl;
			continue;
		}

		assert_elems.add(new Assertion(src_type, predicate, seq++));
	}
}

Assertion::Assertion(
	const Type* in_src_type,
	const CondExpr* in_pred,
	unsigned int	in_seq)
: src_type(in_src_type),
  pred(in_pred->copy()),
  seq(in_seq)
{
	assert (src_type != NULL);
}

Assertion::~Assertion(void)
{
	delete pred;
}

void Assertion::genCode(void)
{
	code_builder->genCodeCond(
		src_type,
		getFCallName(),
		pred,
		new Number(1),	/* assertion is satisfied */
		new Number(0)	/* assertion failed, whoops! */);
}

void Assertion::genProtos(void)
{
	code_builder->genThunkProto(getFCallName());
}


const string Assertion::getFCallName(void) const
{
	return "__assert_" + src_type->getName() + "_" + int_to_string(seq);
}
