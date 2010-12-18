#include <iostream>

#include "runtime_interface.h"
#include "symtab.h"
#include "type.h"
#include "evalctx.h"
#include "code_builder.h"
#include "util.h"
#include "struct_writer.h"
#include "virt.h"

extern CodeBuilder		*code_builder;
extern symtab_map		symtabs;
extern type_map			types_map;
extern RTInterface		rt_glue;

using namespace std;

VirtualTypes::VirtualTypes(const Type* t)
: src_type(t), seq(0)
{
	assert (t != NULL);
	loadVirtuals(false);
	loadVirtuals(true);
}

void VirtualTypes::loadVirtuals(bool is_cond)
{
	preamble_list	virt_preambles;

	virt_preambles = src_type->getPreambles(
		"virtual" + string(((is_cond) ? "_if" : "")));

	for (	preamble_list::const_iterator it = virt_preambles.begin();
		it != virt_preambles.end();
		it++)
	{
		VirtualType			*virt;
		virt = loadVirtual(*it, is_cond);
		if (virt == NULL)
			continue;
		virts.add(virt);
	}
}

VirtualType* VirtualTypes::loadVirtual(const Preamble* p, bool is_conditional)
{
	const preamble_args		*args;
	const Expr			*_target_type_expr;
	const CondExpr			*cond;
	const Id			*target_type_expr, *as_name;
	preamble_args::const_iterator	arg_it;
	EvalCtx				ectx(symtabs[src_type->getName()]);
	const Type			*target_type;
	InstanceIter			*instance_iter;

	args = p->getArgsList();
	if ((is_conditional == false && args->size() != 5) ||
	    (is_conditional == true && args->size() != 6)) {
		cerr << "virt call given wrong number of args" << endl;
		return NULL;
	}

	arg_it = args->begin();
	if (is_conditional) {
		cond = (*arg_it)->getCondExpr();
		assert (cond != NULL);
		arg_it++;
	} else
		cond = NULL;

	_target_type_expr = (*arg_it)->getExpr(); arg_it++;
	target_type_expr = dynamic_cast<const Id*>(_target_type_expr);
	if (target_type_expr == NULL) {
		cerr << "expected id with type name" << endl;
		return NULL;
	}

	if (types_map.count(target_type_expr->getName()) == 0) {
		cerr << "Bad xlated type in virt's first parameter:'";
		_target_type_expr->print(cerr);
		cerr << "' in type " << src_type->getName() << endl;
		return NULL;
	}
	target_type = types_map[target_type_expr->getName()];

	as_name = p->getAddressableName();

	instance_iter = new InstanceIter();
	instance_iter->load(src_type, arg_it);

	if (is_conditional) {
		return new VirtualIf(
			instance_iter,
			cond->copy(),
			target_type,
			(as_name != NULL) ? as_name->copy() : NULL,
			seq++);
	} else {
		return new VirtualType(
			instance_iter,
			target_type,
			(as_name != NULL) ? as_name->copy() : NULL,
			seq++);
	}
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

VirtualIf::VirtualIf(
	InstanceIter	*in_iter,
	CondExpr	*in_cond,
	const Type	*in_virt_type,
	Id		*in_name,
	unsigned int	seq)
: VirtualType(in_iter, in_virt_type, in_name, seq),
  cond(in_cond)
{
	assert (cond != NULL);

	true_min_expr = iter->getMinExpr()->copy();
	false_min_expr = new AOPAdd(
		iter->getMaxExpr()->copy(),
		new Number(1));	/* false_min_expr > max_expr */

	iter->setMinExpr(new FCall(
		new Id(getWrapperFCallName()),
		new ExprList(rt_glue.getThunkClosure())));
}

VirtualIf::~VirtualIf(void)
{
	delete cond;
	delete true_min_expr;
	delete false_min_expr;
}

const string VirtualIf::getWrapperFCallName(void) const
{
	return 	"__virtif_condwrap_" + iter->getSrcType()->getName() + "_" +
		int_to_string(getSeqNum());
}

void VirtualIf::genCode(void) const
{
	code_builder->genCodeCond(
		iter->getSrcType(),
		getWrapperFCallName(),
		cond,true_min_expr, false_min_expr);

	VirtualType::genCode();
}

void VirtualIf::genProto(void) const
{
	code_builder->genThunkProto(getWrapperFCallName());
	VirtualType::genProto();
}

void VirtualType::genInstance(TableGen* tg) const
{
	StructWriter		sw(tg->getOS());
	const InstanceIter	*ii;
	Id			*name;

	ii = getInstanceIter();
	sw.write("vt_type_src", ii->getDstType()->getTypeNum());
	sw.write("vt_type_virttype", getTargetType()->getTypeNum());
	sw.write("vt_range", ii->getLookupFCallName());
	sw.write("vt_min", ii->getMinFCallName());
	sw.write("vt_max", ii->getMaxFCallName());

	name = getName();
	if (name != NULL)	sw.writeStr("vt_name", name->getName());
	else			sw.write("vt_name", "NULL");
}

void VirtualTypes::genTables(TableGen* tg)
{
	StructWriter	sw(
		tg->getOS(),
		"fsl_rtt_virt",
		"__rt_tab_virt_" + getType()->getName() + "[]",
		true);

	for (	virt_list::const_iterator it = virts.begin();
		it != virts.end();
		it++)
	{
		sw.beginWrite();
		(*it)->genInstance(tg);
	}
}

void VirtualTypes::genExterns(TableGen* tg)
{
	for (	virt_list::const_iterator it = virts.begin();
		it != virts.end();
		it++)
	{
		(*it)->genExterns(tg);
	}
}
