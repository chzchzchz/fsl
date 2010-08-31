/* generates tables that tools will use to work with data.  */
#include <iostream>
#include <string>
#include <map>
#include <typeinfo>

#include "struct_writer.h"
#include "AST.h"
#include "type.h"
#include "func.h"
#include "symtab.h"
#include "eval.h"
#include "points_to.h"
#include "asserts.h"
#include "table_gen.h"
#include "thunk_fieldoffset_cond.h"

#include <stdint.h>

extern type_list		types_list;
extern type_map			types_map;
extern const_map		constants;
extern pointing_list		points_list;
extern pointing_map		points_map;
extern assert_map		asserts_map;
extern assert_list		asserts_list;

using namespace std;

void TableGen::genInstanceTypeField(
	const Type*		parent_type,
	const SymbolTableEnt*	st_ent)
{
	StructWriter			sw(out);
	const Type			*field_type;
	const ThunkField		*tf;
	const ThunkFieldOffsetCond	*condoff;
	FCall				*field_off_fc;
	FCall				*field_elems_fc;
	FCall				*field_size_fc;
	FCall				*field_params_fc;

	tf = st_ent->getFieldThunk();
	condoff = dynamic_cast<const ThunkFieldOffsetCond*>(tf->getOffset());
	field_off_fc = tf->getOffset()->copyFCall();
	field_elems_fc = tf->getElems()->copyFCall();
	field_size_fc = tf->getSize()->copyFCall();
	field_params_fc = tf->getParams()->copyFCall();
	field_type = types_map[st_ent->getTypeName()];

	sw.writeStr("tf_fieldname", st_ent->getFieldName());
	sw.write("tf_fieldbitoff", field_off_fc->getName());
	if (field_type != NULL)
		sw.write("tf_typenum", field_type->getTypeNum());
	else
		sw.write("tf_typenum", "~0");
	sw.write("tf_elemcount", field_elems_fc->getName());
	sw.write("tf_typesize", field_size_fc->getName());
	sw.write("tf_params",  field_params_fc->getName());

	if (condoff != NULL) {
		FCall	*present_fc;

		present_fc = condoff->copyFCallPresent();
		sw.write("tf_cond", present_fc->getName());
		delete present_fc;
	} else {
		sw.write("tf_cond", "NULL");
	}

	sw.writeB("tf_constsize", tf->getSize()->isConstant());

	delete field_params_fc;
	delete field_off_fc;
	delete field_elems_fc;
	delete field_size_fc;
}

void TableGen::genThunksTableBySymtab(
	const string&		table_name,
	const SymbolTable&	st)
{
	StructWriter	sw(
		out, 
		"fsl_rt_table_field", 
		table_name + "[]",
		true);

	for (	sym_list::const_iterator it = st.begin();
		it != st.end();
		it++)
	{
		const SymbolTableEnt	*st_ent;
		
		st_ent = *it;
		sw.beginWrite();
		genInstanceTypeField(st.getOwnerType(), st_ent);
	}

}

void TableGen::genUserFieldsByType(const Type *t)
{
	SymbolTable	*st;
	SymbolTable	*st_all;
	SymbolTable	*st_types;

	st = t->getSymsByUserTypeStrong();
	genThunksTableBySymtab(
		string("__rt_tab_thunks_") + t->getName(),
		*st);
	delete st;

	st_all = t->getSymsStrongOrConditional();
	genThunksTableBySymtab(
		string("__rt_tab_thunksall_") + t->getName(),
		*st_all);
	delete st_all;

	st_types = t->getSymsByUserTypeStrongOrConditional();
	genThunksTableBySymtab(
		string("__rt_tab_thunkstypes_") + t->getName(),
		*st_types);
	delete st_types;
}

void TableGen::genUserFieldTables(void)
{
	for (	type_list::const_iterator it = types_list.begin();
		it != types_list.end();
		it++)
		genUserFieldsByType(*it);
}

void TableGen::genInstanceType(const Type *t)
{
	StructWriter	sw(out);
	SymbolTable	*st, *st_all, *st_types;
	FCall		*size_fc;
	unsigned int	pointsto_count;
	unsigned int	assert_count;

	st = t->getSymsByUserTypeStrong();
	st_all = t->getSymsStrongOrConditional();
	st_types = t->getSymsByUserTypeStrongOrConditional();

	assert (st != NULL);

	size_fc = st->getThunkType()->getSize()->copyFCall();
	pointsto_count = points_map[t->getName()]->getNumPointing();
	assert_count = asserts_map[t->getName()]->getNumAsserts();

	sw.writeStr("tt_name", t->getName());
	sw.write("tt_param_c", t->getNumArgs());
	sw.write("tt_size", size_fc->getName());
	sw.write("tt_field_c", st->size());
	sw.write("tt_field_thunkoff", "__rt_tab_thunks_" + t->getName());
	
	sw.write("tt_pointsto_c", pointsto_count);
	sw.write("tt_pointsto", "__rt_tab_pointsto_" + t->getName());

	sw.write("tt_assert_c", assert_count);
	sw.write("tt_assert", "__rt_tab_asserts_" + t->getName());

	sw.write("tt_fieldall_c", st_all->size());
	sw.write("tt_fieldall_thunkoff", "__rt_tab_thunksall_" + t->getName());

	sw.write("tt_fieldtypes_c", st_types->size());
	sw.write("tt_fieldtypes_thunkoff","__rt_tab_thunkstypes_"+t->getName());

	delete size_fc;
	delete st_types;
	delete st_all;
	delete st;
}

void TableGen::printExternFunc(
	const string& fname,
	const vector<string>& args,
	const char* return_type)
{
	vector<string>::const_iterator it = args.begin();

	out << "extern " << return_type << ' ' << fname << '(';

	out << *it;
	for (	it++;
		it != args.end();
		it++)
	{
		out << ", ";
		out << *it;
	}
	out << ");\n";
}

void TableGen::printExternFuncThunk(
	const std::string& funcname,
	const char* return_type)
{
	const string args[] = {"uint64_t", "uint64_t*"};
	printExternFunc(
		funcname, 
		vector<string>(args, args+2),
		return_type);
}

void TableGen::printExternFuncThunkParams(const ThunkParams* tp)
{
	const string args[] = {"uint64_t", "uint64_t*", "uint64_t*"};
	FCall	*fc;

	fc = tp->copyFCall();
	printExternFunc(
		fc->getName(),
		vector<string>(args, args+3),
		"void");
	delete fc;
}

void TableGen::genExternsFieldsByType(const Type *t)
{
	SymbolTable	*st;
	FCall		*fc_type_size;

	st = t->getSyms();
	assert (st != NULL);

	for (	sym_list::const_iterator it = st->begin();
		it != st->end();
		it++)
	{
		const SymbolTableEnt		*st_ent;
		const ThunkField		*tf;
		const ThunkFieldOffsetCond	*fo_cond;
		FCall				*fc_off;
		FCall				*fc_elems;
		FCall				*fc_size;

		st_ent = *it;
		tf = st_ent->getFieldThunk();

		fc_off = tf->getOffset()->copyFCall();
		fc_elems = tf->getElems()->copyFCall();
		fc_size = tf->getSize()->copyFCall();

		printExternFuncThunk(fc_off->getName());
		printExternFuncThunk(fc_elems->getName());
		printExternFuncThunk(fc_size->getName());
		printExternFuncThunkParams(tf->getParams());

		fo_cond = dynamic_cast<const ThunkFieldOffsetCond*>(
			tf->getOffset());
		if (fo_cond != NULL) {
			FCall	*cond_fc;
			cond_fc = fo_cond->copyFCallPresent();
			printExternFuncThunk(cond_fc->getName(), "bool");
			delete cond_fc;
		}

		out << endl;

		delete fc_off;
		delete fc_elems;
		delete fc_size;
	}

	fc_type_size = st->getThunkType()->getSize()->copyFCall();
	printExternFuncThunk(fc_type_size->getName());
	delete fc_type_size;
}

void TableGen::genExternsFields(void)
{
	for (	type_list::const_iterator it = types_list.begin();
		it != types_list.end();
		it++)
		genExternsFieldsByType(*it);
}

void TableGen::genTable_fsl_rt_table(void)
{
	StructWriter	sw(out, "fsl_rt_table_type", "fsl_rt_table[]");

	for (	type_list::iterator it = types_list.begin();
		it != types_list.end();
		it++)
	{
		sw.beginWrite();
		genInstanceType(*it);
	}
}

void TableGen::genTableHeaders(void)
{
	out << "#include <stdint.h>" << endl;
	out << "#include \"../runtime.h\"" << endl;
}

void TableGen::genInstanceAssertion(const Assertion* as)
{
	StructWriter	sw(out);
	sw.write("as_assertf", as->getFCallName());
}

void TableGen::genInstancePointsRange(const PointsRange* ptr)
{
	StructWriter	sw(out);

	sw.write("pt_type_dst", ptr->getDstType()->getTypeNum());
	sw.write("pt_range", ptr->getFCallName());
	sw.write("pt_min", ptr->getMinFCallName());
	sw.write("pt_max", ptr->getMaxFCallName());
}

void TableGen::genExternsAsserts(const Asserts* as)
{
	const assertion_list*	asl = as->getAsserts();

	for (	assertion_list::const_iterator it = asl->begin();
		it != asl->end();
		it++)
	{
		const Assertion*	assertion = *it;
		printExternFuncThunk(assertion->getFCallName(), "bool");
	}
}

void TableGen::printExternPointsRange(const PointsRange* pr)
{
	string	args_pr[] = {"uint64_t", "uint64_t*", "uint64_t", "uint64_t*"};
	string	args_bound[] = {"uint64_t", "uint64_t*"};

	printExternFunc(
		pr->getFCallName(),
		vector<string>(args_pr,args_pr+4),
		"uint64_t");

	printExternFunc(
		pr->getMinFCallName(),
		vector<string>(args_bound,args_bound+2),
		"uint64_t");

	printExternFunc(
		pr->getMaxFCallName(),
		vector<string>(args_bound,args_bound+2),
		"uint64_t");
}

void TableGen::genExternsPoints(const Points* pt)
{
	const pointsto_list*	pt_list = pt->getPointsTo();
	const pointsrange_list*	ptr_list = pt->getPointsRange();

	for (	pointsto_list::const_iterator it = pt_list->begin();
		it != pt_list->end();
		it++)
	{
		printExternPointsRange(*it);
	}

	for (	pointsrange_list::const_iterator it = ptr_list->begin();
		it != ptr_list->end();
		it++)
	{
		printExternPointsRange(*it);
	}
}


void TableGen::genPointsTable(const Points* pt)
{
	StructWriter	sw(
		out,
		"fsl_rt_table_pointsto",
		"__rt_tab_pointsto_" + pt->getType()->getName() + "[]",
		true);
	const pointsto_list*	pt_list = pt->getPointsTo();
	const pointsrange_list*	ptr_list = pt->getPointsRange();


	for (	pointsto_list::const_iterator it = pt_list->begin();
		it != pt_list->end();
		it++)
	{
		sw.beginWrite();
		genInstancePointsRange(*it);
	}

	for (	pointsrange_list::const_iterator it = ptr_list->begin();
		it != ptr_list->end();
		it++)
	{
		sw.beginWrite();
		genInstancePointsRange(*it);
	}
}

void TableGen::genPointsTables(void)
{
	for (	pointing_list::const_iterator it = points_list.begin();
		it != points_list.end();
		it++)
	{
		genExternsPoints(*it);
		genPointsTable(*it);
	}
}

void TableGen::genAssertsTables(void)
{
	for (	assert_list::const_iterator it = asserts_list.begin();
		it != asserts_list.end();
		it++)
	{
		genExternsAsserts(*it);
		genAssertsTable(*it);
	}
}

void TableGen::genAssertsTable(const Asserts* as)
{
	StructWriter	sw(
		out,
		"fsl_rt_table_assert",
		"__rt_tab_asserts_" + as->getType()->getName() + "[]",
		true);
	const assertion_list	*al = as->getAsserts();

	for (	assertion_list::const_iterator it = al->begin();
		it != al->end();
		it++)
	{
		sw.beginWrite();
		genInstanceAssertion(*it);
	}
}

void TableGen::genScalarConstants(void)
{
	const Type	*origin_type;

	origin_type = types_map["disk"];
	assert (origin_type != NULL);

	out << "unsigned int fsl_rt_table_entries = "<<types_list.size()<<";\n";
	out << "unsigned int fsl_rt_origin_typenum = ";
	out << origin_type->getTypeNum() << ';' << endl;

	out << "char fsl_rt_fsname[] = \"";
	if (constants.count("__FSL_FSNAME") == 0) 
		out << "__FSL_FSNAME";
	else {
		const Id*	id;
		id = dynamic_cast<const Id*>(constants["__FSL_FSNAME"]);
		out << ((id != NULL) ? id->getName() : "__FSL_FSNAME");
	}
	out << "\";" << endl;
}

void TableGen::gen(const string& fname)
{
	out.open(fname.c_str());

	genTableHeaders();
	genScalarConstants();

	genExternsFields();
	genUserFieldTables();
	genPointsTables();
	genAssertsTables();

	genTable_fsl_rt_table();

	out.close();
}
