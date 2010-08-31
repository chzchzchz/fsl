#ifndef TABLEGEN_H
#define TABLEGEN_H

#include <iostream>
#include <fstream>
#include <fstream>
#include <string>

class TableGen
{
public:
	TableGen() {}
	virtual ~TableGen() {}

	void gen(const std::string& fname = "fsl.table.c");
private:
	void genTableHeaders(void);

	void genPointsTables(void);
	void genPointsTable(const Points* pt);

	void genAssertsTables(void);
	void genAssertsTable(const Asserts* as);

	void genTable_fsl_rt_table(void);
	void genExternsFieldsByType(const Type *t);
	void genExternsFields(void);
	void genThunksTableBySymtab(
		const std::string&	table_name,
		const SymbolTable&	st);
	void genUserFieldsByType(const Type *t);
	void genUserFieldTables(void);

	void printExternFunc(
		const std::string& fname,	
		const std::vector<std::string>& args,
		const char* return_type);

	void printExternFuncThunk(
		const std::string& funcname,
		const char* return_type = "uint64_t");

	void printExternPointsRange(const PointsRange*);

	/* these generate the struct instances ala
	 * { .some_field = whatever, ... } */
	void genInstanceType(const Type *t);
	void genInstanceTypeField(
		const Type*		parent_type,
		const SymbolTableEnt*	st_ent);
	void genInstancePointsRange(const PointsRange* ptr);

	void genInstanceAssertion(const Assertion* assertion);

	void genExternsAsserts(const Asserts* as);
	void genExternsPoints(const Points* pt);
	void genScalarConstants(void);

	std::ofstream	out;
};

#endif
