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
#include "points_to.h"

#include <stdint.h>
#include <fstream>

extern type_list		types_list;
extern type_map			types_map;
extern pointing_list		points_list;
extern pointing_map		points_map;

using namespace std;

static void gen_rt_table_typefield(
	ostream& 		out, 
	const Type*		parent_type,
	const SymbolTableEnt*	st_ent)
{
	StructWriter	sw(out);
	const Type		*field_type;
	const ThunkField	*tf;
	FCall			*field_off_fc;
	FCall			*field_elems_fc;
	FCall			*field_size_fc;

	tf = st_ent->getFieldThunk();
	field_off_fc = tf->getOffset()->copyFCall();
	field_elems_fc = tf->getElems()->copyFCall();
	field_size_fc = tf->getSize()->copyFCall();
	field_type = types_map[st_ent->getTypeName()];

	sw.writeStr("tt_fieldname", st_ent->getFieldName());
	sw.write("tt_fieldbitoff", field_off_fc->getName());
	if (field_type != NULL)
		sw.write("tt_typenum", field_type->getTypeNum());
	else
		sw.write("tt_typenum", "~0");
	sw.write("tt_elemcount", field_elems_fc->getName());
	sw.write("tt_typesize", field_size_fc->getName());

	delete field_off_fc;
	delete field_elems_fc;
	delete field_size_fc;
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
	unsigned int	pointsto_num;

	st = t->getSymsByUserTypeStrong();
	assert (st != NULL);

	size_fc = st->getThunkType()->getSize()->copyFCall();
	pointsto_num = points_map[t->getName()]->getNumPointing();

	sw.writeStr("tt_name", t->getName());
	sw.write("tt_size", size_fc->getName());
	sw.write("tt_num_fields", st->size());
	sw.write("tt_field_thunkoff", "__rt_tab_thunks_" + t->getName());
	sw.write("tt_num_pointsto", pointsto_num);
	sw.write("tt_pointsto", "__rt_tab_pointsto_" + t->getName());

	delete size_fc;
	delete st;
}

static void print_extern_func(
	ostream& out,
	const string& funcname,
	unsigned int num_params = 1)
{
	out << "extern uint64_t " << funcname << "(";
	
	for (unsigned int i = 0; i < num_params; i++) {
		out << "uint64_t";
		if (i != (num_params - 1)) {
			out << ", ";
		}
	}
	
	out << ");\n";
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
		const ThunkField	*tf;
		FCall			*fc_off;
		FCall			*fc_elems;
		FCall			*fc_size;

		st_ent = (*it).second;
		tf = st_ent->getFieldThunk();

		fc_off = tf->getOffset()->copyFCall();
		fc_elems = tf->getElems()->copyFCall();
		fc_size = tf->getSize()->copyFCall();

		print_extern_func(out, fc_off->getName());
		print_extern_func(out, fc_elems->getName());
		print_extern_func(out, fc_size->getName());

		delete fc_off;
		delete fc_elems;
		delete fc_size;
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

static void gen_fsl_rt_pointsto(ostream& out, const PointsTo* pto)
{
	StructWriter	sw(out);

	sw.write("pt_type_dst", pto->getDstType()->getTypeNum());
	sw.write("pt_single", pto->getFCallName());
	sw.write("pt_range", "NULL");
	sw.write("pt_min", "NULL");
	sw.write("pt_max", "NULL");
}

static void gen_fsl_rt_pointsrange(ostream& out, const PointsRange* ptr)
{
	StructWriter	sw(out);

	sw.write("pt_type_dst", ptr->getDstType()->getTypeNum());
	sw.write("pt_single", "NULL");
	sw.write("pt_range", ptr->getFCallName());
	sw.write("pt_min", ptr->getMinFCallName());
	sw.write("pt_max", ptr->getMaxFCallName());
}

static void gen_rt_externs_pointsto(ostream& out, const Points* pt)
{
	const pointsto_list*	pt_list = pt->getPointsTo();
	const pointsrange_list*	ptr_list = pt->getPointsRange();


	for (	pointsto_list::const_iterator it = pt_list->begin();
		it != pt_list->end();
		it++)
	{
		print_extern_func(out, (*it)->getFCallName());
	}

	for (	pointsrange_list::const_iterator it = ptr_list->begin();
		it != ptr_list->end();
		it++)
	{
		const PointsRange*	ptr = *it;
		print_extern_func(out, ptr->getFCallName(), 2);
		print_extern_func(out, ptr->getMinFCallName(), 1);
		print_extern_func(out, ptr->getMaxFCallName(), 1);
	}
}


static void gen_fsl_rt_tab_pointsto(ostream& out, const Points* pt)
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
		gen_fsl_rt_pointsto(out, *it);
	}

	for (	pointsrange_list::const_iterator it = ptr_list->begin();
		it != ptr_list->end();
		it++)
	{
		sw.beginWrite();
		gen_fsl_rt_pointsrange(out, *it);
	}
}

static void gen_fsl_rt_tab_pointsto_all(ostream& out)
{
	for (	pointing_list::const_iterator it = points_list.begin();
		it != points_list.end();
		it++)
	{
		gen_rt_externs_pointsto(out, *it);
		gen_fsl_rt_tab_pointsto(out, *it);
	}
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
	gen_fsl_rt_tab_pointsto_all(out);

	gen_fsl_rt_table(out);
	
}

