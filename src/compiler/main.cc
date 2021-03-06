#include <iostream>
#include <string>
#include <map>
#include <typeinfo>
#include "AST.h"
#include "type.h"
#include "func.h"
#include "deftype.h"
#include "symtab.h"
#include "eval.h"
#include "code_builder.h"
#include "symtab_thunkbuilder.h"
#include "asserts.h"
#include "repair.h"
#include "points.h"
#include "virt.h"
#include "stat.h"
#include "table_gen.h"
#include "detached_preamble.h"
#include "runtime_interface.h"
#include "writepkt.h"
#include "reloc.h"
#include "memotab.h"
#include "util.h"

#include <stdint.h>
#include <fstream>

extern int yyparse();

extern GlobalBlock* global_scope;

using namespace std;

/* annotations */
typereloc_map		typerelocs_map;
typereloc_list		typerelocs_list;
func_map		funcs_map;
func_list		funcs_list;
writepkt_map		writepkts_map;
writepkt_list		writepkts_list;
pointing_list		points_list;
pointing_map		points_map;
assert_map		asserts_map;
assert_list		asserts_list;
stat_map		stats_map;
stat_list		stats_list;
typevirt_map		typevirts_map;
typevirt_list		typevirts_list;
repair_list		repairs_list;
repair_map		repairs_map;

const Func		*gen_func;
const FuncBlock		*gen_func_block;
const char		*fsl_src_fname;
MemoTab			memotab;
CodeBuilder		*code_builder;
RTInterface		rt_glue;

ctype_map		ctypes_map;
type_map		types_map;
type_list		types_list;
typenum_map		typenums_map;
const_map		constants;
symtab_map		symtabs;
deftype_map		deftypes_map;

static void	load_detached_preambles(const GlobalBlock* gb);
static void	load_user_types_list(const GlobalBlock* gb);
static void	load_primitive_ptypes(void);
static void	load_def_types(const GlobalBlock* gb);
static bool	apply_consts_to_consts(void);
static void	simplify_constants(void);

static void load_primitive_ptypes(void)
{
#define PRIM_NUM	3
	const char *prim_name[] = {"bool", "int", "uint"};
	int	prim_bits[] = {1, 64, 64};

	for (unsigned int i = 0; i < PRIM_NUM; i++)
		ctypes_map[prim_name[i]] = prim_bits[i];

	for (unsigned int i = 1; i <= 64; i++)
		ctypes_map["u"+int_to_string(i)] = i;
}

static void load_detached_preambles(const GlobalBlock* gb)
{
	for (auto &block : *gb) {
		auto dp = dynamic_cast<DetachedPreamble*>(block.get());
		if (dp == NULL) continue;

		auto t = types_map[dp->getTypeName()];
		if (t == NULL) {
			cerr	<< "Detached preamble with bad type (" <<
				dp->getTypeName() << ")" << endl;
			continue;
		}

		t->addPreamble(dp->getPreamble());
	}
}

static void load_def_types(const GlobalBlock* gb)
{
	for (auto &block : *gb) {
		auto dt = dynamic_cast<DefType*>(block.get());
		if (dt == NULL) continue;

		if (ctypes_map.count(dt->getName()) != 0) {
			cerr << "Trying to remap primitive type." << endl;
			continue;
		}

		if (deftypes_map.count(dt->getName()) != 0) {
			cerr << "Typedef " << dt->getName() << " already defined"
				<< endl;
			continue;
		}

		if (deftypes_map.count(dt->getTargetTypeName()) != 0) {
			cerr << "Oops, no nesting typedefs: " << dt->getTargetTypeName()
				<< endl;
			continue;
		}

		if (ctypes_map.count(dt->getTargetTypeName()) == 0) {
			cerr	<< "Oops, typedef '"
				<< dt->getName()
				<< "' target needs to be a primitive type. Not "
				<< dt->getTargetTypeName()
				<< endl;
			continue;
		}

		deftypes_map[dt->getName()] = dt;
		ctypes_map[dt->getName()] = ctypes_map[dt->getTargetTypeName()];
	}
}

static void load_user_types_list(const GlobalBlock* gb)
{
	int type_num = 0;

	for (auto &block : *gb) {
		auto t = dynamic_cast<Type*>(block.get());
		if (t == NULL) continue;

		types_list.push_back(t);
		types_map[t->getName()] = t;

		t->setTypeNum(type_num);
		typenums_map[type_num] = t;
		type_num++;
	}

	code_builder->createGlobalConst("fsl_num_types", type_num);
	/* remember to set these in the run-time */
	code_builder->createGlobalMutable("__FROM_OS_BDEV_BYTES", 0);
	code_builder->createGlobalMutable("__FROM_OS_BDEV_BLOCK_BYTES", 0);
	code_builder->createGlobalMutable("__FROM_OS_SB_BLOCKSIZE_BYTES", 0);
}

static void load_user_funcs(const GlobalBlock* gb)
{
	for (auto &block : *gb) {
		auto f = dynamic_cast<Func*>(block.get());
		if (f == NULL) continue;

		/* add to mappings.. */
		funcs_list.push_back(f);
		funcs_map[f->getName()] = f;
		f->genProto();

		if (memotab.canMemoize(f)) memotab.registerFunction(f);
	}

	memotab.allocTable();

	for (auto f : funcs_list) f->genCode();
}

/**
 * build up symbol tables, disambiguate if possible
 */
static void build_symtabs(void)
{
	for (auto t : types_list) {
		SymbolTable	*syms;

		assert (t != NULL);

		cout << "Building symtab for " << t->getName() << endl;

		t->buildSyms();
		syms = t->getSyms();
		symtabs[t->getName()] = syms;
	}
}

static void load_constants(const GlobalBlock* gb)
{
	constants["true"] = new Boolean(true);
	constants["false"] = new Boolean(false);

	for (auto &block : *gb) {
		auto c = dynamic_cast<ConstVar*>(block.get());
		if (c == NULL) continue;
		constants[c->getName()] = (c->getExpr())->copy();
	}
}

static void load_enums(const GlobalBlock* gb)
{
	for (auto &block : *gb) {
		unsigned long	n;

		auto e = dynamic_cast<Enum*>(block.get());
		if (e == NULL) continue;

		/* map values to enum */
		n = 0;
		for (const auto& ent : *e) {
			const Expr *ent_num = ent->getNumber();

			constants[ent->getName()] = (ent_num == nullptr)
				? new Number(n)
				: ent_num->copy();
			
			n++;
		}

		/* alias enum's type into fixed-width type */
		if (ctypes_map.count(e->getTypeName()) == 0) {
			cerr	<< "Could not resolve enum physical type \""
				<< e->getTypeName() << '"' << endl;
			continue;
		}

		ctypes_map[e->getName()] = ctypes_map[e->getTypeName()];
	}
}

#define MAX_PASSES	100

static void dump_constants(void)
{
	for (auto &p : constants) {
		cerr << p.first << ": ";
		p.second->print(cerr);
		cerr << endl;
	}
}

static void simplify_constants(void)
{
	unsigned int pass;

	pass = 0;
	while (pass < MAX_PASSES) {
		if (!apply_consts_to_consts()) return;
		pass++;
	}

	cerr << "Could not resolve constants after " << pass <<
		"passes -- circular?" << endl;
	cerr << "Dumping: " << endl;
	dump_constants();

	assert (0 == 1);
}

static bool apply_consts_to_consts(void)
{
	bool updated = false;

	for (auto &p : constants) {
		Expr	*new_expr, *old_expr;

		old_expr = (p.second)->copy();
		new_expr = expr_resolve_consts(constants, p.second);

		if (*old_expr != new_expr) {
			constants[p.first] = new_expr;
			updated = true;
		}

		delete old_expr;
	}

	return updated;
}

static void gen_thunk_code(void)
{
	for (auto &p : symtabs) {
		const SymbolTable	*st = p.second;
		st->getThunkType()->genCode();
	}
}

static void gen_thunk_proto(void)
{
	for (auto &p : symtabs) {
		const SymbolTable	*st = p.second;
		st->getThunkType()->genProtos();
	}
}

template<class NoteType>
static void gen_notes(
	PtrList<NoteType> &note_list,
	std::map<std::string, NoteType*> &note_map
	)
{
	for (const auto& t : types_list) {
		NoteType	*note = new NoteType(t);
		note->genProto();
		note->genCode();
		note_list.add(note);
		note_map[t->getName()] = note;
	}
}

template<class NoteType>
static void gen_notes(
	std::list<NoteType*> &note_list,
	std::map<std::string, NoteType*> &note_map
	)
{
	for (const auto& t : types_list) {
		NoteType	*note = new NoteType(t);
		note->genProto();
		note->genCode();
		note_list.push_back(note);
		note_map[t->getName()] = note;
	}
}

static void gen_writepkts(const GlobalBlock* gb)
{
	for (auto &block : *gb) {
		auto wpkt = dynamic_cast<WritePkt*>(block.get());
		if (wpkt == NULL) continue;

		/* add to mappings.. */
		writepkts_list.push_back(wpkt);
		writepkts_map[wpkt->getName()] = wpkt;
		wpkt->genProtos();
	}

	for (auto &wpkt : writepkts_list) wpkt->genCode();
}

int main(int argc, char *argv[])
{
	TableGen		table_gen;
	char			*out_str;
	string			table_fname;
	string			llvm_fname;

	if (argc < 2) {
		printf("Usage: %s outputstr\n", argv[0]);
		return 0;
	}
	out_str = argv[1];
	llvm_fname = string("fsl.") + out_str + string(".types.ll");
	table_fname = string("fsl.") + out_str + string(".table.c");

	code_builder = new CodeBuilder("fsl.types.mod");

	if (argc == 3) {
		FILE	*f = freopen(argv[2], "r", stdin);
		assert(f != nullptr && "couldn't open provided file");
	}

	fsl_src_fname = "TOP_FILE";
	yyparse();

	thunkbuilder_init_funcmap();

	/* create prototypes for functions provided by the run-time */
	rt_glue.loadRunTimeFuncs(code_builder);

	/* load ptypes in */
	cout << "Loading ptypes" << endl;
	load_primitive_ptypes();

	cout << "Loading typedefs" << endl;
	load_def_types(global_scope);

	/* make consts resolve to numbers */
	cout << "Setting up constants" << endl;
	load_constants(global_scope);
	load_enums(global_scope);
	cout << "Simplifying constants" << endl;
	simplify_constants();

	cout << "Loading user types list" << endl;
	load_user_types_list(global_scope);

	cout << "Attaching detached preambles" << endl;
	load_detached_preambles(global_scope);

	/* next, build up symbol tables on types.. this is our type checking */
	cout << "Building symbol tables" << endl;
	build_symtabs();

	cout << "Generating thunk prototypes" << endl;
	gen_thunk_proto();

	/* generate function prototyes so that we can resolve thunks within
	 * functions */
	cout << "Loading user functions" << endl;
	load_user_funcs(global_scope);

	cerr << "Generating write packets" << endl;
	gen_writepkts(global_scope);

	cout << "Loading thunks" << endl;
	gen_thunk_code();

	cout << "Generating points-to functions" << endl;
	gen_notes<Points>(points_list, points_map);

	cout << "Generating assertions" << endl;
	gen_notes<Asserts>(asserts_list, asserts_map);

	cout << "Generating virtuals" << endl;
	gen_notes<VirtualTypes>(typevirts_list, typevirts_map);

	cout << "Generating relocations" << endl;
	gen_notes<RelocTypes>(typerelocs_list, typerelocs_map);

	cout << "Generating stats" << endl;
	gen_notes<Stat>(stats_list, stats_map);

	cout << "Generating repairs" << endl;
	gen_notes<Repairs>(repairs_list, repairs_map);

	cout << "Writing out module's code" << endl;
	code_builder->write(llvm_fname);

	cout << "Generating fsl.table.c" << endl;
	table_gen.gen(table_fname);

	return 0;
}
