#include <assert.h>
#include <iostream>
#include "type.h"
#include "reloc.h"
#include "writepkt.h"
#include "instance_iter.h"
#include "util.h"

using namespace std;

RelocTypes::RelocTypes(const Type* t)
: src_type(t), seq(0)
{
	preamble_list	pl;

	assert (t != NULL);

	pl = src_type->getPreambles("relocate");
	for (	preamble_list::const_iterator it = pl.begin();
		it != pl.end();
		it++)
	{
		Reloc	*r = loadReloc(*it);
		if (r == NULL) continue;
		relocs.push_back(r);
		seq++;
	}
}


Reloc* RelocTypes::loadReloc(const Preamble* p)
{
	const preamble_args		*args;
	preamble_args::const_iterator	arg_it;
	const CondExpr			*choice_cond;
	InstanceIter			*sel_iter, *choice_iter;
	WritePktInstance		*wi_alloc, *wi_relink, *wi_replace;

	args = p->getArgsList();
	/* 2 instance iters, 1 cond, three writepkts */
	if (args->size() != (2*4+1+3)) {
		cerr << "reloc given wrong number of args" << endl;
		return NULL;
	}

	arg_it = args->begin();

	sel_iter = new InstanceIter();
	choice_iter = new InstanceIter();
	sel_iter->load(src_type, arg_it);
	choice_iter->load(src_type, arg_it);

	choice_cond = (*arg_it)->getCondExpr(); arg_it++;
	if (choice_cond == NULL) {
		cerr << "reloc expected conditional but got expr ";
		((*arg_it)->getExpr())->print(cerr);
		cerr << endl;
		delete sel_iter;
		delete choice_iter;
		return NULL;
	}

	wi_alloc = WritePkt::getInstance(
		(*arg_it)->getExpr(),
		"reloc_alloc_"+src_type->getName()+"_"+int_to_string(seq),
		src_type);
	arg_it++;
	wi_relink = WritePkt::getInstance(
		(*arg_it)->getExpr(),
		"reloc_relink_"+src_type->getName()+"_"+int_to_string(seq),
		src_type);
	arg_it++;
	wi_replace = WritePkt::getInstance(
		(*arg_it)->getExpr(),
		"reloc_replace_"+src_type->getName()+"_"+int_to_string(seq),
		src_type);

	assert (wi_alloc && wi_relink && wi_replace);

	return new Reloc(
		p->getAddressableName(),
		sel_iter, choice_iter,
		choice_cond->copy(),
		wi_alloc, wi_relink, wi_replace);
}

Reloc::Reloc(
	const Id* in_as_name,
	InstanceIter*	in_sel_iter,
	InstanceIter*	in_choice_iter,
	CondExpr*	in_choice_cond,
	WritePktInstance	*in_wpkt_alloc,
	WritePktInstance	*in_wpkt_relink,
	WritePktInstance	*in_wpkt_replace
	)
:	sel_iter(in_sel_iter), choice_iter(in_choice_iter),
	choice_cond(in_choice_cond),
	wpkt_alloc(in_wpkt_alloc),
	wpkt_relink(in_wpkt_relink),
	wpkt_replace(in_wpkt_replace)
{
	as_name = (in_as_name != NULL) ? in_as_name->copy() : NULL;
}

Reloc::~Reloc()
{
	if (as_name != NULL) delete as_name;
	delete sel_iter;
	delete choice_iter;
	delete choice_cond;
	delete wpkt_alloc;
	delete wpkt_relink;
	delete wpkt_replace;
}

void Reloc::genCode(void) const
{
	ArgsList	*args;

	args = new ArgsList();
	args->add(new Id("u64"), choice_iter->getBinding()->copy());
	wpkt_alloc->genCode(args);

	args->clear();
	args->add(new Id("u64"), sel_iter->getBinding()->copy());
	args->add(new Id("u64"), choice_iter->getBinding()->copy());
	wpkt_relink->genCode(args);

	args->clear();
	args->add(new Id("u64"), sel_iter->getBinding()->copy());
	wpkt_replace->genCode(args);

	delete args;
}

void Reloc::genProtos(void) const
{
	wpkt_alloc->genProto();
	wpkt_relink->genProto();
	wpkt_replace->genProto();
}

void RelocTypes::genProtos(void)
{
	for (	reloc_list::const_iterator it = relocs.begin();
		it != relocs.end();
		it++)
	{
		(*it)->genProtos();
	}
}

void RelocTypes::genCode(void)
{
	for (	reloc_list::const_iterator it = relocs.begin();
		it != relocs.end();
		it++)
	{
		(*it)->genCode();
	}
}

void RelocTypes::genExterns(TableGen* tg)
{
	for (	reloc_list::const_iterator it = relocs.begin();
		it != relocs.end();
		it++)
	{
		(*it)->genExterns(tg);
	}
}

void RelocTypes::genTables(TableGen* tg)
{
	cerr << "herp" << endl;
}

void Reloc::genExterns(TableGen* tg) const
{
	cerr << "derp" << endl;
}
