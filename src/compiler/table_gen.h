#ifndef TABLEGEN_H
#define TABLEGEN_H

#include <iostream>
#include <fstream>
#include <fstream>
#include <string>
#include "collection.h"
#include "symtab.h"

class TableGen;

class TableWriter
{
public:
	virtual void genExterns(TableGen* tg) = 0;
	virtual void genTables(TableGen* tg) = 0;
	virtual ~TableWriter() {}
protected:
	TableWriter() {}
};

typedef std::list<TableWriter*> tablewriter_list;

class TableGen
{
public:
	TableGen() {}
	virtual ~TableGen() {}

	void gen(const std::string& fname = "fsl.table.c");
	void printExternFunc(
		const std::string& fname,
		const char* return_type,
		const std::vector<std::string>& args);

	void printExternFuncThunk(
		const std::string& funcname,
		const char* return_type = "uint64_t");
	void printExternFuncThunkParams(const class ThunkParams* tp);
	std::ofstream& getOS(void) { return out; }
private:
	template<class T>
	void genTableWriters(const PtrList<T>& tw_list);

	template<class T>
	void genTableWriters(const std::list<T*>& tw_list);


	void genTableHeaders(void);

	void genWritePktTables(void);
	void genWritePktTable(const class WritePkt* wpkt);

	void genTable_fsl_rt_table(void);
	void genExternsFieldsByType(const Type *t);
	void genExternsFields(void);
	void genExternsUserFuncs(void);
	void genThunksTableBySymtab(
		const std::string&	table_name,
		const SymbolTable&	st);
	void genUserFieldsByType(const Type *t);
	void genUserFieldTables(void);

	/* these generate the struct instances ala
	 * { .some_field = whatever, ... } */
	void genInstanceType(const Type *t);

	void genScalarConstants(void);

	std::ofstream	out;
};

#endif
