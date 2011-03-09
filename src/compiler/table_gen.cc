/* generates tables that tools will use to work with data.  */
#include <iostream>
#include <string>
#include <map>
#include <typeinfo>

#include "util.h"
#include "struct_writer.h"
#include "AST.h"
#include "type.h"
#include "func.h"
#include "symtab.h"
#include "eval.h"
#include "points.h"
#include "asserts.h"
#include "stat.h"
#include "virt.h"
#include "reloc.h"
#include "writepkt.h"
#include "table_gen.h"
#include "thunk_fieldoffset_cond.h"
#include "memotab.h"

#include <stdint.h>

extern type_list		types_list;
extern type_map			types_map;
extern func_list		funcs_list;
extern const_map		constants;
extern pointing_list		points_list;
extern pointing_map		points_map;
extern assert_map		asserts_map;
extern assert_list		asserts_list;
extern stat_map			stats_map;
extern stat_list		stats_list;
extern typevirt_map		typevirts_map;
extern typevirt_list		typevirts_list;
extern writepkt_list		writepkts_list;
extern typereloc_map		typerelocs_map;
extern typereloc_list		typerelocs_list;
extern MemoTab			memotab;

using namespace std;


void TableGen::genThunksTableBySymtab(
	const string&		table_name,
	const SymbolTable&	st)
{
	StructWriter	sw(out, "fsl_rtt_field", table_name + "[]", true);

	for (	sym_list::const_iterator it = st.begin();
		it != st.end();
		it++)
	{
		const SymbolTableEnt	*st_ent;
		st_ent = *it;
		sw.beginWrite();
		st_ent->getFieldThunk()->genFieldEntry(this);
	}
}

void TableGen::genUserFieldsByType(const Type *t)
{
	SymbolTable	*st;
	SymbolTable	*st_all;
	SymbolTable	*st_types;
	SymbolTable	*st_complete;

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

	st_complete = t->getSyms();
	genThunksTableBySymtab(
		string("__rt_tab_thunkcomplete_") + t->getName(),
		*st_complete);
	delete st_complete;
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
	SymbolTable	*st, *st_all, *st_types, *st_complete;
	FCall		*size_fc;
	const string	tname(t->getName());

	st = t->getSymsByUserTypeStrong();
	st_all = t->getSymsStrongOrConditional();
	st_types = t->getSymsByUserTypeStrongOrConditional();
	st_complete = t->getSyms();

	assert (st != NULL);

	size_fc = st->getThunkType()->getSize()->copyFCall();

	sw.writeStr("tt_name", tname);
	sw.write("tt_param_c", t->getParamBufEntryCount());
	sw.write("tt_arg_c", t->getNumArgs());
	sw.write("tt_size", size_fc->getName());
	sw.write("tt_fieldstrong_c", st->size());
	sw.write("tt_fieldstrong_table", "__rt_tab_thunks_" + tname);

	sw.write("tt_pointsto_c", points_map[tname]->getNumPointing());
	sw.write("tt_pointsto", "__rt_tab_pointsto_" + tname);

	sw.write("tt_assert_c", asserts_map[tname]->getNumAsserts());
	sw.write("tt_assert", "__rt_tab_asserts_" + tname);

	sw.write("tt_stat_c", stats_map[tname]->getNumStat());
	sw.write("tt_stat", "__rt_tab_stats_"+tname);

	sw.write("tt_reloc_c", typerelocs_map[tname]->getNumRelocs());
	sw.write("tt_reloc", "__rt_tab_reloc_" + tname);

	sw.write("tt_fieldall_c", st_all->size());
	sw.write("tt_fieldall_thunkoff", "__rt_tab_thunksall_" + tname);

	sw.write("tt_fieldtypes_c", st_types->size());
	sw.write("tt_fieldtypes_thunkoff","__rt_tab_thunkstypes_"+tname);

	sw.write("tt_virt_c", typevirts_map[tname]->getNumVirts());
	sw.write("tt_virt", "__rt_tab_virt_" + tname);

	sw.write("tt_field_c", st_complete->size());
	sw.write("tt_field_table", "__rt_tab_thunkcomplete_" + tname);

	delete size_fc;
	delete st_complete;
	delete st_types;
	delete st_all;
	delete st;
}

void TableGen::printExternFunc(
	const string& fname,
	const char* return_type,
	const vector<string>& args)
{
	vector<string>::const_iterator it = args.begin();

	out << "extern " << return_type << ' ' << fname << '(' << (*it);
	for (it++; it != args.end(); it++) out << ", " << (*it);
	out << ");\n";
}

void TableGen::printExternFuncThunk(
	const std::string& funcname,
	const char* return_type)
{
	const string args[] = {"const struct fsl_rt_closure*"};
	printExternFunc(
		funcname,
		return_type,
		vector<string>(args, args+1));
}

void TableGen::printExternFuncThunkParams(const ThunkParams* tp)
{
	const string args[] = {
		"const struct fsl_rt_closure*", /* parent pointer */
		"uint64_t",	/* idx */
		"uint64_t*",
		};
	FCall	*fc;

	fc = tp->copyFCall();
	printExternFunc(fc->getName(), "void", vector<string>(args, args+3));
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
		st_ent = *it;
		st_ent->getFieldThunk()->genFieldExtern(this);
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

void TableGen::genExternsUserFuncs(void)
{
	for (	func_list::const_iterator it = funcs_list.begin();
		it != funcs_list.end();
		it++)
	{
		const Func	*f = *it;
		if (!memotab.canMemoize(f)) continue;
		out <<	"extern uint64_t " << (*it)->getName() <<
			"(void);" << endl;
	}
}

void TableGen::genTable_fsl_rt_table(void)
{
	StructWriter	sw(out, "fsl_rtt_type", "fsl_rt_table[]");

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
	out << "#include \"runtime/runtime.h\"" << endl;
}

template<class T>
void TableGen::genTableWriters(const list<T*>& tw_list)
{
	for (	typename list<T*>::const_iterator it = tw_list.begin();
		it != tw_list.end();
		it++)
	{
		(*it)->genExterns(this);
		(*it)->genTables(this);
	}
}

void TableGen::genScalarConstants(void)
{
	const Type	*origin_type;

	origin_type = types_map["disk"];
	assert (origin_type != NULL
		&& "No origin type 'disk' declared");
	assert (origin_type->getNumArgs() == 0
		&& "Type 'disk' should not take parameters");

	out << "unsigned int fsl_rtt_entries = "<<types_list.size()<<";\n";
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

	out << "int __fsl_mode = ";
	if (constants.count("__FSL_MODE") == 0)
		out << "0"; /* mode little endian */
	else {
		const Number*	num;
		num = dynamic_cast<const Number*>(constants["__FSL_MODE"]);
		out << ((num != NULL) ? num->getValue() : 0);
	}
	out << ";" << endl;

}

void TableGen::genWritePktTables(void)
{
	writepkt_list::const_iterator it;

	for (it = writepkts_list.begin(); it != writepkts_list.end(); it++)
		(*it)->genExterns(this);

	for (it = writepkts_list.begin(); it != writepkts_list.end(); it++)
		(*it)->genTables(this);
}

void TableGen::gen(const string& fname)
{
	out.open(fname.c_str());

	genTableHeaders();
	genScalarConstants();

	genExternsFields();
	genExternsUserFuncs();
	genUserFieldTables();
	genTableWriters<Points>(points_list);
	genTableWriters<Asserts>(asserts_list);
	genTableWriters<VirtualTypes>(typevirts_list);
	genTableWriters<Stat>(stats_list);
	genWritePktTables();
	genTableWriters<RelocTypes>(typerelocs_list);

	memotab.genTables(this);

	genTable_fsl_rt_table();

	out.close();
}
