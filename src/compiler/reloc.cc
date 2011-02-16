#include <assert.h>
#include <iostream>
#include "type.h"
#include "reloc.h"
#include "writepkt.h"
#include "wpkt_inst.h"
#include "instance_iter.h"
#include "struct_writer.h"
#include "code_builder.h"
#include "evalctx.h"
#include "cond.h"
#include "util.h"

using namespace std;
extern CodeBuilder	*code_builder;
extern symtab_map	symtabs;

RelocTypes::RelocTypes(const Type* t) : Annotation(t, "relocate")
{
	loadByName();
}

void RelocTypes::load(const Preamble* p)
{
	Reloc	*r = loadReloc(p);
	if (r == NULL) return;
	relocs.add(r);
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
		"reloc_alloc_"+src_type->getName()+"_"+int_to_string(getSeq()),
		src_type);
	assert (wi_alloc != NULL);
	arg_it++;
	wi_relink = WritePkt::getInstance(
		(*arg_it)->getExpr(),
		"reloc_relink_"+src_type->getName()+"_"+int_to_string(getSeq()),
		src_type);
	((*arg_it)->getExpr())->print(cerr);
	assert (wi_relink != NULL);
	arg_it++;
	wi_replace = WritePkt::getInstance(
		(*arg_it)->getExpr(),
		"reloc_replace_"+src_type->getName()+"_"+int_to_string(getSeq()),
		src_type);
	assert (wi_replace != NULL);

	sel_iter->setPrefix(
		"reloc_sel_"+src_type->getName()+"_"+int_to_string(getSeq()));
	choice_iter->setPrefix(
		"reloc_choice_"+src_type->getName()+"_"+int_to_string(getSeq()));

	return new Reloc(
		this,
		p,
		sel_iter, choice_iter,
		choice_cond->copy(),
		wi_alloc, wi_relink, wi_replace);
}

Reloc::Reloc(
	RelocTypes*	in_parent,
	const Preamble* in_pre,
	InstanceIter*	in_sel_iter,
	InstanceIter*	in_choice_iter,
	CondExpr*	in_choice_cond,
	WritePktInstance	*in_wpkt_alloc,
	WritePktInstance	*in_wpkt_relink,
	WritePktInstance	*in_wpkt_replace)
:	AnnotationEntry(in_parent, in_pre),
	sel_iter(in_sel_iter), choice_iter(in_choice_iter),
	choice_cond(in_choice_cond),
	wpkt_alloc(in_wpkt_alloc),
	wpkt_relink(in_wpkt_relink),
	wpkt_replace(in_wpkt_replace)
{ }

Reloc::~Reloc()
{
	delete sel_iter;
	delete choice_iter;
	delete choice_cond;
	delete wpkt_alloc;
	delete wpkt_relink;
	delete wpkt_replace;
}

void Reloc::genCondCode(void) const
{
	llvm::Function		*f;
	llvm::BasicBlock	*bb_entry;
	llvm::Value		*cond_v;
	llvm::IRBuilder<>	*builder;
	llvm::LLVMContext	&gctx(llvm::getGlobalContext());
	EvalCtx			ectx(symtabs[parent->getType()->getName()]);
	ArgsList		args;

	builder = code_builder->getBuilder();
	f = code_builder->getModule()->getFunction(getCondFuncName());

	bb_entry = llvm::BasicBlock::Create(gctx, "entry", f);
	builder->SetInsertPoint(bb_entry);

	args.add(new Id("u64"), choice_iter->getBinding()->copy());
	code_builder->genThunkHeaderArgs(f, parent->getType(), &args);

	cond_v = cond_codeGen(&ectx, choice_cond);
	if (cond_v == NULL) {
		cerr << getCondFuncName() <<
			": could not gen condition" << endl;
		return;
	}

	builder->CreateRet(cond_v);
}

void Reloc::genCode(void)
{
	ArgsList	*args;

	sel_iter->genCode();
	choice_iter->genCode();

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

	genCondCode();
}

void Reloc::genProto(void)
{
	wpkt_alloc->genProto();
	wpkt_relink->genProto();
	wpkt_replace->genProto();
	genCondProto();
	sel_iter->genProto();
	choice_iter->genProto();
}

string Reloc::getCondFuncName(void) const
{
	return string("reloc_cond_")+parent->getType()->getName()+
		int_to_string(seq);
}

void Reloc::genCondProto(void) const
{
	llvm::Function		*f;
	llvm::FunctionType	*ft;
	std::string		f_name;

	f_name = getCondFuncName();

	/* write pkt function calls take a single argument, a pointer */
	vector<const llvm::Type*>	args;

	args.push_back(code_builder->getClosureTyPtr());
	args.push_back(llvm::Type::getInt64Ty(llvm::getGlobalContext()));

	ft = llvm::FunctionType::get(
		llvm::Type::getInt1Ty(llvm::getGlobalContext()),
		args,
		false);

	f = llvm::Function::Create(
		ft,
		llvm::Function::ExternalLinkage,
		f_name,
		code_builder->getModule());

	/* should not be redefinitions.. */
	if (f->getName() != f_name) {
		cerr << "Expected name " << f_name <<" got " <<
		f->getNameStr() << endl;
	}

	assert (f->getName() == f_name);
	assert (f->arg_size() == 2);
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
	StructWriter	sw(
		tg->getOS(),
		"fsl_rtt_reloc",
		"__rt_tab_reloc_" + getType()->getName() + "[]",
		true);

	for (	reloc_list::const_iterator it = relocs.begin();
		it != relocs.end();
		it++)
	{
		sw.beginWrite();
		(*it)->genTableInstance(tg);
	}
}

void Reloc::genTableInstance(TableGen* tg) const
{
	StructWriter		sw(tg->getOS());
	const Id		*as_name(getName());

	sw.write(".rel_sel = ");
	sel_iter->genTableInstance(tg);
	sw.write(".rel_choice = ");
	choice_iter->genTableInstance(tg);

	sw.write("rel_ccond", getCondFuncName());

	sw.write(".rel_alloc = ");
	wpkt_alloc->genTableInstance(tg);
	sw.write(".rel_replace = ");
	wpkt_replace->genTableInstance(tg);
	sw.write(".rel_relink = ");
	wpkt_relink->genTableInstance(tg);

	if (as_name != NULL)	sw.writeStr("rel_name", as_name->getName());
	else			sw.write("rel_name", "NULL");
}

void Reloc::genExterns(TableGen* tg) const
{
	sel_iter->printExterns(tg);
	choice_iter->printExterns(tg);
	wpkt_alloc->genExterns(tg);
	wpkt_relink->genExterns(tg);
	wpkt_replace->genExterns(tg);

	vector<string>	cond_args;
	cond_args.push_back("const struct fsl_rt_closure*");
	cond_args.push_back("uint64_t idx");
	tg->printExternFunc(getCondFuncName(), cond_args, "bool");
}
