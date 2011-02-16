#include <assert.h>
#include "symtab.h"
#include "code_builder.h"
#include "struct_writer.h"
#include "util.h"

#include "asserts.h"

using namespace std;

extern CodeBuilder		*code_builder;
extern symtab_map		symtabs;

Asserts::Asserts(const Type* t) : Annotation(t, "assert") { loadByName(); }
Asserts::~Asserts(void) {}

void Asserts::load(const Preamble* p)
{
	const preamble_args	*args;
	const CondExpr		*predicate;

	args = p->getArgsList();
	if (args == NULL || args->size() != 1) {
		cerr << "assert: expects one argument" << endl;
		return;
	}

	predicate = (args->front())->getCondExpr();
	if (predicate == NULL) {
		cerr << "assert: expects conditional expression" << endl;
		return;
	}

	assert_elems.add(new Assertion(this, p, predicate));
}

Assertion::Assertion(
	Asserts*	in_parent,
	const Preamble* in_pre,
	const CondExpr* in_pred)
: AnnotationEntry(in_parent, in_pre) { pred = in_pred->copy(); }

Assertion::~Assertion(void) { delete pred; }

void Assertion::genCode(void)
{
	code_builder->genCodeCond(
		parent->getType(),
		getFCallName(),
		pred,
		new Number(1),	/* assertion is satisfied */
		new Number(0)	/* assertion failed, whoops! */);
}

void Assertion::genProto(void) { code_builder->genThunkProto(getFCallName()); }

void Assertion::genInstance(TableGen* tg) const
{
	StructWriter	sw(tg->getOS());
	const Id	*name;
	sw.write("as_assertf", getFCallName());
	name = getName();
	if (name != NULL)	sw.writeStr("as_name", name->getName());
	else			sw.write("as_name", "NULL");
}

void Asserts::genExterns(TableGen* tg)
{
	for (	assertion_list::const_iterator it = assert_elems.begin();
		it != assert_elems.end();
		it++)
	{
		tg->printExternFuncThunk((*it)->getFCallName(), "bool");
	}
}

void Asserts::genTables(TableGen* tg)
{
	StructWriter	sw(
		tg->getOS(),
		"fsl_rtt_assert",
		"__rt_tab_asserts_" + getType()->getName() + "[]",
		true);

	for (	assertion_list::const_iterator it = assert_elems.begin();
		it != assert_elems.end();
		it++)
	{
		sw.beginWrite();
		(*it)->genInstance(tg);
	}
}
