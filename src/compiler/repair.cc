#include <iostream>
#include "code_builder.h"
#include "repair.h"
#include "wpkt_inst.h"

using namespace std;

extern CodeBuilder* code_builder;

Repairs::Repairs(const Type* t) : Annotation(t, "repair") { loadByName(); }

void Repairs::load(const Preamble* p)
{
	const preamble_args		*args;
	preamble_args::const_iterator	arg_it;
	const CondExpr			*cond;
	WritePktInstance		*wpkt_repair;

	/* repair(predicate, writepacket) */
	if ((args = p->getArgsList())->size() != 2) {
		cerr << "Bad repair args.." << endl;
		return;
	}

	arg_it = args->begin();

	cond = (*arg_it)->getCondExpr(); arg_it++;
	if (cond == NULL) {
		cerr << "reloc expected conditional but got expr ";
		((*arg_it)->getExpr())->print(cerr);
		cerr << endl;
		return;
	}

	wpkt_repair = WritePkt::getInstance(
		(*arg_it)->getExpr(),
		"repairwpkt_"+src_type->getName()+"_"+int_to_string(getSeq()),
		src_type);
	assert (wpkt_repair != NULL);

	repairs_l.push_back(new RepairEnt(this, p, cond,wpkt_repair));
}

void Repairs::genExterns(TableGen* tg)
{
	for (	repairent_list::const_iterator it = repairs_l.begin();
		it != repairs_l.end();
		it++)
		(*it)->genExterns(tg);
}

void Repairs::genTables(TableGen* tg)
{
	StructWriter	sw(
		tg->getOS(),
		"fsl_rtt_repair",
		"__rt_tab_repair_" + getType()->getName() + "[]",
		true);

	for (	repairent_list::const_iterator it = repairs_l.begin();
		it != repairs_l.end();
		it++)
	{
		sw.beginWrite();
		(*it)->genTableInstance(tg);
	}
}

RepairEnt::RepairEnt(
	Repairs* in_parent, const Preamble* in_pre,
	const CondExpr* ce, WritePktInstance* in_wpkt)
 : 	AnnotationEntry(in_parent, in_pre),
	cond_expr(ce),
	repair_wpkt(in_wpkt)
{ }

RepairEnt::~RepairEnt(void) { delete repair_wpkt; }

void RepairEnt::genCode(void)
{
	repair_wpkt->genCode();
	genCondCode();
}

void RepairEnt::genProto(void)
{
	repair_wpkt->genProto();
	genCondProto();
}

void RepairEnt::genCondCode(void) const
{
	Expr		*n0 = new Number(0), *n1 = new Number(1);
	code_builder->genCodeCond(
		parent->getType(), getCondFuncName(), cond_expr, n1, n0);
	delete n0;
	delete n1;
}

void RepairEnt::genCondProto(void) const
{
	code_builder->genThunkProto(getCondFuncName());
}

void RepairEnt::genExterns(TableGen* tg) const
{
	repair_wpkt->genExterns(tg);
	tg->printExternFuncThunk(getCondFuncName(), "bool");
}

void RepairEnt::genTableInstance(TableGen* tg) const
{
	StructWriter		sw(tg->getOS());

	sw.write("rep_cond", getCondFuncName());
	sw.write(".rep_wpkt = ");
	repair_wpkt->genTableInstance(tg);
	writeName(sw, "rep_name");
}

std::string RepairEnt::getCondFuncName(void) const
{
	return string("rep_cond_")+
		parent->getType()->getName()+int_to_string(seq);
}