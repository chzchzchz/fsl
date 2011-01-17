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
#include "virt.h"
#include "reloc.h"
#include "writepkt.h"
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
extern typevirt_map		typevirts_map;
extern typevirt_list		typevirts_list;
extern writepkt_list		writepkts_list;
extern typereloc_map		typerelocs_map;
extern typereloc_list		typerelocs_list;

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

	sw.write("tf_flags",
		((tf->getElems()->isNoFollow() == true) ? 4 : 0) |
		((tf->getElems()->isFixed() == true) ? 2 : 0) |
		((tf->getSize()->isConstant() == true) ? 1 : 0));

	sw.write("tf_fieldnum", tf->getFieldNum());

	delete field_params_fc;
	delete field_off_fc;
	delete field_elems_fc;
	delete field_size_fc;
}

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
		genInstanceTypeField(st.getOwnerType(), st_ent);
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
	const string args[] = {"const struct fsl_rt_closure*"};
	printExternFunc(
		funcname,
		vector<string>(args, args+1),
		return_type);
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
}

void TableGen::genExternsWriteStmts(const WritePkt* wpkt)
{
	WritePkt::const_iterator	it;
	const string args[] = {"const uint64_t*"};

	for (it = wpkt->begin(); it != wpkt->end(); it++) {
		const WritePktBlk	*wblk = *it;

		for (	WritePktBlk::const_iterator it2 = wblk->begin();
			it2 != wblk->end();
			it2++)
		{
			const WritePktStmt	*wstmt = *it2;
			printExternFunc(
				wstmt->getFuncName(),
				vector<string>(args, args+1),
				"void");
		}
	}
}

void TableGen::genWritePktTable(const WritePkt* wpkt)
{
	WritePkt::const_iterator	it;
	unsigned int			n;
	unsigned int			num_blks;

	/* generate dump of write stmts in a few tables */
	for (it = wpkt->begin(), n = 0; it != wpkt->end(); it++, n++) {
		const WritePktBlk	*wblk = *it;

		out << "static wpktf_t wpkt_funcs_" <<
			wpkt->getName() << n << "[] = \n";

		{
		StructWriter			sw(out);
		for (	WritePktBlk::const_iterator it2 = wblk->begin();
			it2 != wblk->end();
			it2++)
		{
			const WritePktStmt	*wstmt = *it2;
			sw.write(wstmt->getFuncName());
		}
		}
		out << ";\n";
	}

	list<WritePktBlk*>	l(*wpkt);

	reverse(l.begin(), l.end());
	/* now generate writepkt structs.. */
	num_blks = wpkt->size();
	for (it = l.begin(), n = num_blks-1; it != l.end(); it++, n--) {
		const WritePktBlk*	wblk = *it;
		StructWriter	sw(
			out,
			"fsl_rtt_wpkt",
			string("wpkt_") + wpkt->getName() + int_to_string(n),
			true);
		sw.write("wpkt_param_c",
			wpkt->getArgs()->getNumParamBufEntries());
		sw.write("wpkt_func_c", wblk->size());
		sw.write("wpkt_funcs",
			"wpkt_funcs_" + wpkt->getName() + int_to_string(n));
		/* XXX need to support embedded blocks.. */
		sw.write("wpkt_blk_c", 0);
		sw.write("wpkt_blks", "NULL");

		if (n != 0)
			sw.write(
				"wpkt_next",
				string("wpkt_")+wpkt->getName()+
					int_to_string(n-1));
		else
			sw.write("wpkt_next", "NULL");
	}
}

void TableGen::genWritePktTables(void)
{
	writepkt_list::const_iterator it;

	for (it = writepkts_list.begin(); it != writepkts_list.end(); it++)
		genExternsWriteStmts(*it);

	for (it = writepkts_list.begin(); it != writepkts_list.end(); it++)
		genWritePktTable(*it);
}

void TableGen::gen(const string& fname)
{
	out.open(fname.c_str());

	genTableHeaders();
	genScalarConstants();

	genExternsFields();
	genUserFieldTables();
	genTableWriters<Points>(points_list);
	genTableWriters<Asserts>(asserts_list);
	genTableWriters<VirtualTypes>(typevirts_list);
	genWritePktTables();
	genTableWriters<RelocTypes>(typerelocs_list);

	genTable_fsl_rt_table();

	out.close();
}
