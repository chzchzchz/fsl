#include <iostream>

#include "symtab.h"
#include "type.h"
#include "evalctx.h"
#include "virt.h"

extern symtab_map		symtabs;
extern type_map			types_map;

using namespace std;

VirtualTypes::VirtualTypes(const Type* t)
: src_type(t), seq(0)
{
	assert (t != NULL);
	loadVirtuals();
}

void VirtualTypes::loadVirtuals(void)
{
	preamble_list	virt_preambles;

	virt_preambles = src_type->getPreambles("virtual");
	for (	preamble_list::const_iterator it = virt_preambles.begin();
		it != virt_preambles.end();
		it++)
	{
		VirtualType			*virt;
		virt = loadVirtual(*it);
		if (virt == NULL)
			continue;
		virts.add(virt);
	}
}

VirtualType* VirtualTypes::loadVirtual(const Preamble* p)
{
	const preamble_args		*args;
	const Expr			*_target_type_expr, *_binding,
					*min_expr, *max_expr,
					*lookup_expr;
	const Id			*binding, *target_type_expr;
	preamble_args::const_iterator	arg_it;
	EvalCtx				ectx(symtabs[src_type->getName()]);
	const Type			*xlated_type;
	const Type			*target_type;

	args = p->getArgsList();
	if (args->size() != 5) {
		cerr << "virtual expected 5 args" << endl;
		return NULL;
	}

	arg_it = args->begin();
	_target_type_expr = (*arg_it)->getExpr(); arg_it++;
	_binding = (*arg_it)->getExpr(); arg_it++;
	min_expr = (*arg_it)->getExpr(); arg_it++;
	max_expr = (*arg_it)->getExpr(); arg_it++;
	lookup_expr = (*arg_it)->getExpr(); arg_it++;

	binding = dynamic_cast<const Id*>(_binding);
	if (binding == NULL) {
		cerr << "expected id for binding" << endl;
		return NULL;
	}

	target_type_expr = dynamic_cast<const Id*>(_target_type_expr);
	if (target_type_expr == NULL) {
		cerr << "expected id with type name" << endl;
		return NULL;
	}



	if (types_map.count(target_type_expr->getName()) == 0) {
		cerr << "Bad xlated type in virt's first parameter:'";
		lookup_expr->print(cerr);
		cerr << "' in type " << src_type->getName() << endl;
		return NULL;
	}
	target_type = types_map[target_type_expr->getName()];

	xlated_type = ectx.getType(lookup_expr);
	if (xlated_type == NULL) {
		cerr << "Bad xlated type in virt's last parameter:'";
		lookup_expr->print(cerr);
		cerr << "' in type " << src_type->getName() << endl;
		return NULL;
	}

	return new VirtualType(
		src_type,
		xlated_type,
		target_type,
		binding->copy(),
		min_expr->copy(), max_expr->copy(),
		lookup_expr->copy(),
		seq++);
}

void VirtualTypes::genCode(void)
{
	for (	virt_list::const_iterator it = virts.begin();
		it != virts.end();
		it++)
	{
		(*it)->genCode();
	}
}

void VirtualTypes::genProtos(void)
{
	for (	virt_list::const_iterator it = virts.begin();
		it != virts.end();
		it++)
	{
		(*it)->genProto();
	}
}

const string VirtualType::getFCallName(void) const
{
	return PointsRange::getFCallName() + "_virt";
}

const string VirtualType::getMinFCallName(void) const
{
	return PointsRange::getMinFCallName() + "_virt";
}

const string VirtualType::getMaxFCallName(void) const
{
	return PointsRange::getMaxFCallName() + "_virt";
}
