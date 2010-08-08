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

	void gen(const char* fname = "fsl.table.c");
private:
	void genTableHeaders(void);

	void genPointsTables(void);
	void genPointsTable(const Points* pt);

	void genTable_fsl_rt_table(void);
	void genExternsFieldsByType(const Type *t);
	void genExternsFields(void);
	void genThunksTableBySymtab(
		const std::string&	table_name,
		const SymbolTable&	st);
	void genUserFieldsByType(const Type *t);
	void genUserFieldTables(void);

	void printExternFunc(
		const std::string& funcname,
		unsigned int num_params = 1);

	/* these generate the struct instances ala
	 * { .some_field = whatever, ... } */
	void genInstanceType(const Type *t);
	void genInstanceTypeField(
		const Type*		parent_type,
		const SymbolTableEnt*	st_ent);
	void genInstancePointsTo(const PointsTo* pto);
	void genInstancePointsRange(const PointsRange* ptr);


	void genExternsPoints(const Points* pt);
	void genIntConstants(void);

	std::ofstream	out;
};

#endif
