/**
 * generates tables that tools will use to workw ith data. 
 */
#include <iostream>
#include <string>
#include <map>
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
	const Type	*field_type;
	unsigned int	typenum;
	FCall		*field_off_fc;

	out << "{";

	/* tt_fieldname */
	out << '"' << st_ent->getFieldName() << "\",";

	 /*tt_fieldbitoff */
	field_off_fc = st_ent->getFieldThunk()->getOffset()->copyFCall();
	out  << field_off_fc->getName() << ',';
	delete field_off_fc;

	/* tt_typenum */
	field_type = types_map[st_ent->getTypeName()];
	if (field_type != NULL) 
		out << field_type->getTypeNum();
	else
		out << "~0";

	out << "}";
}

static void gen_rt_tab_thunks_by_type(ostream& out, const Type *t)
{
	SymbolTable	*st;
	bool		print_comma;	

	st = t->getSymsByUserType();

	out << "static struct fsl_rt_table_thunk __rt_tab_thunks_"<<t->getName()<<"[] = ";
	out << "{\n";
	print_comma = false;
	for (	sym_map::const_iterator it = st->begin();
		it != st->end();
		it++)
	{
		if (print_comma) out << ',' << endl;
		else print_comma = true;

		gen_rt_table_typefield(out, t, (*it).second);
	}
	out << "};\n";

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
	SymbolTable	*st;
	FCall		*size_fc;
	bool		print_comma;

	st = t->getSymsByUserType();
	assert (st != NULL);

	size_fc = st->getThunkType()->getSize()->copyFCall();
	out << "\"" << t->getName() << "\", ";	/* tt_name */
	out << size_fc->getName() << ", ";	/* tt_sizef */
	out << st->size() << ',';		/* tt_num_fields */

	delete size_fc;

	/* tt_field_thunkoff */
	out << "__rt_tab_thunks_" << t->getName() << endl;

	delete st;
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

		st_ent = (*it).second;
		fc_off = st_ent->getFieldThunk()->getOffset()->copyFCall();
		out 	<< "extern uint64_t " 
			<< fc_off->getName() 
			<< "(uint64_t);" << endl;
		delete fc_off;
	}

	fc_type_size = st->getThunkType()->getSize()->copyFCall();
	out << "extern uint64_t " << fc_type_size->getName() 
		<< "(uint64_t);" << endl;
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
	bool	print_comma;

	out << "unsigned int fsl_rt_table_entries = " << types_list.size() << ";";
	out << endl;
	out << "struct fsl_rt_table_type fsl_rt_table[] = {" << endl;

	print_comma = false;
	for (	type_list::iterator it = types_list.begin();
		it != types_list.end();
		it++)
	{
		if (print_comma)
			out << "," << endl << endl;
		else
			print_comma = true;
		out << "{";
		gen_rt_table_type(out, *it);
		out << "}";
	}
	
	out << "};" << endl;

}

void gen_rt_tables(void)
{
	ofstream	out("fsl.table.c");
	bool		print_comma;
	const Type	*origin_type;

	out << "#include <stdint.h>" << endl;
	out << "#include \"../runtime.h\"" << endl;

	gen_rt_externs(out);

	origin_type = types_map["disk"];
	assert (origin_type != NULL);

	out << "unsigned int fsl_rt_origin_typenum = ";
	out << origin_type->getTypeNum() << ';' << endl;

	gen_fsl_rt_tab_thunks(out);
	gen_fsl_rt_table(out);
}
