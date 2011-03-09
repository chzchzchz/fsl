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
 : Annotation(t, "virtual")
{
	loadByName();
	loadByName("virtual_if");
}

void VirtualTypes::load(const Preamble* pre)
{
	VirtualType	*virt;
	bool		is_cond;

	is_cond = (pre->getName() == string("virtual_if"));

	virt = loadVirtual(pre, is_cond);
	if (virt == NULL) return;
	virts.add(virt);
}

VirtualType* VirtualTypes::loadVirtual(const Preamble* p, bool is_conditional)
{
	const preamble_args		*args;
	const Expr			*_target_type_expr;
	const CondExpr			*cond;
	const Id			*target_type_expr;
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

	instance_iter = new InstanceIter();
	instance_iter->load(src_type, arg_it);

	if (is_conditional) {
		return new VirtualIf(
			this,
			p,
			instance_iter,
			cond->copy(),
			target_type);
	} else {
		return new VirtualType(this, p, instance_iter, target_type);
	}
}

VirtualIf::VirtualIf(
	Annotation	*in_parent,
	const Preamble	*in_pre,
	InstanceIter	*in_iter,
	CondExpr	*in_cond,
	const Type	*in_virt_type)
: VirtualType(in_parent, in_pre, in_iter, in_virt_type),
  cond(in_cond)
{
	assert (cond != NULL);

	true_min_expr = iter->getMinExpr()->copy();
	false_min_expr = new Number(~0);	/* false_min_expr > max_expr */

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

void VirtualIf::genCode(void)
{
	code_builder->genCodeCond(
		iter->getSrcType(),
		getWrapperFCallName(),
		cond, true_min_expr, false_min_expr);

	VirtualType::genCode();
}

void VirtualIf::genProto(void)
{
	code_builder->genThunkProto(getWrapperFCallName());
	VirtualType::genProto();
}

void VirtualType::genInstance(TableGen* tg) const
{
	StructWriter		sw(tg->getOS());
	const InstanceIter	*ii;

	ii = getInstanceIter();
	sw.write(".vt_iter = ");
	ii->genTableInstance(tg);
	sw.write("vt_type_virttype", getTargetType()->getTypeNum());
	writeName(sw, "vt_name");
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
