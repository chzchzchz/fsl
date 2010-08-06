/**
 * generates tables that tools will use to work with data. 
 */
#include <iostream>
#include <string>
#include <map>
#include "struct_writer.h"
#include "AST.h"
#include "type.h"
#include "func.h"
#include "symtab.h"
#include "eval.h"

#include <stdint.h>
#include <fstream>

extern type_list		types_list;
extern type_map			types_map;

using namespace std;

static void gen_rt_table_typefield(
	ostream& 		out, 
	const Type*		parent_type,
	const SymbolTableEnt*	st_ent)
{
	StructWriter	sw(out);
	const Type	*field_type;
	FCall		*field_off_fc;
	FCall		*field_elems_fc;

	field_off_fc = st_ent->getFieldThunk()->getOffset()->copyFCall();
	field_elems_fc = st_ent->getFieldThunk()->getElems()->copyFCall();
	field_type = types_map[st_ent->getTypeName()];

	sw.writeStr("tt_fieldname", st_ent->getFieldName());
	sw.write("tt_fieldbitoff", field_off_fc->getName());
	if (field_type != NULL)
		sw.write("tt_typenum", field_type->getTypeNum());
	else
		sw.write("tt_typenum", "~0");
	sw.write("tt_elemcount", field_elems_fc->getName());

	delete field_off_fc;
	delete field_elems_fc;
}

static void gen_rt_tab_thunks_by_type(ostream& out, const Type *t)
{
	SymbolTable	*st;
	StructWriter	sw(
		out, 
		"fsl_rt_table_thunk", 
		"__rt_tab_thunks_" + t->getName() + "[]",
		true);

	st = t->getSymsByUserTypeStrong();

	for (	sym_map::const_iterator it = st->begin();
		it != st->end();
		it++)
	{
		const SymbolTableEnt	*st_ent;
		
		st_ent = (*it).second;
		assert (st_ent->isWeak() == false);

		sw.beginWrite();
		gen_rt_table_typefield(out, t, (*it).second);
	}

	delete st;
}

static void gen_fsl_rt_tab_thunks(ostream& out)
{
	for (	type_list::const_iterator it = types_list.begin();
		it != types_list.end();
		it++)
	{
		gen_rt_tab_thunks_by_type(out, *it);
	}
}

static void gen_rt_table_type(ostream& out, const Type *t)
{
	StructWriter	sw(out);
	SymbolTable	*st;
	FCall		*size_fc;

	st = t->getSymsByUserTypeStrong();
	assert (st != NULL);

	size_fc = st->getThunkType()->getSize()->copyFCall();

	sw.writeStr("tt_name", t->getName());
	sw.write("tt_size", size_fc->getName());
	sw.write("tt_num_fields", st->size());
	sw.write("tt_field_thunkoff", "__rt_tab_thunks_" + t->getName());

	delete size_fc;
	delete st;
}

static void print_extern_func(
	ostream& out,
	const string& funcname)
{
	out << "extern uint64_t " << funcname << "(uint64_t);\n";
}

static void gen_rt_externs_type(ostream& out, const Type *t)
{
	SymbolTable	*st;
	FCall		*fc_type_size;

	st = t->getSymsByUserType();
	assert (st != NULL);

	for (	sym_map::const_iterator it = st->begin();
		it != st->end();
		it++)
	{
		const SymbolTableEnt	*st_ent;
		FCall			*fc_off;
		FCall			*fc_elems;

		st_ent = (*it).second;

		fc_off = st_ent->getFieldThunk()->getOffset()->copyFCall();
		fc_elems = st_ent->getFieldThunk()->getElems()->copyFCall();

		print_extern_func(out, fc_off->getName());
		print_extern_func(out, fc_elems->getName());

		delete fc_off;
		delete fc_elems;
	}

	fc_type_size = st->getThunkType()->getSize()->copyFCall();
	print_extern_func(out, fc_type_size->getName());
	delete fc_type_size;
}

static void gen_rt_externs(ostream& out)
{
	for (	type_list::const_iterator it = types_list.begin();
		it != types_list.end();
		it++)
	{
		const Type	*t = *it;
		gen_rt_externs_type(out, t);
	}
}

static void gen_fsl_rt_table(ostream& out)
{
	StructWriter	sw(out, "fsl_rt_table_type", "fsl_rt_table[]");

	for (	type_list::iterator it = types_list.begin();
		it != types_list.end();
		it++)
	{
		sw.beginWrite();
		gen_rt_table_type(out, *it);
	}
}

static void gen_rt_tables_headers(ostream& out)
{
	out << "#include <stdint.h>" << endl;
	out << "#include \"../runtime.h\"" << endl;
}

void gen_rt_tables(void)
{
	ofstream	out("fsl.table.c");
	const Type	*origin_type;

	gen_rt_tables_headers(out);

	gen_rt_externs(out);

	origin_type = types_map["disk"];
	assert (origin_type != NULL);

	out << "unsigned int fsl_rt_table_entries = "<<types_list.size()<<";\n";
	out << "unsigned int fsl_rt_origin_typenum = ";
	out << origin_type->getTypeNum() << ';' << endl;

	gen_fsl_rt_tab_thunks(out);
	gen_fsl_rt_table(out);
}

