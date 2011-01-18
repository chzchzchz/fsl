/**
 * Handles memoization for functions.
 */
#ifndef MEMOTAB_H
#define MEMOTAB_H

#include <string>
#include <list>
#include <map>

class MemoTab
{
public:
	typedef std::map<const std::string, int>	funcidx_map;
	typedef std::list<std::string>			funcidx_list;
	MemoTab(void) : memo_table(NULL) {}
	virtual ~MemoTab(void) {}
	void registerFunction(const class Func* f);
	void allocTable(void);
	class llvm::Value* memoFuncCall(const class Func* f) const;
	bool canMemoize(const class Func* f) const;
	void genTables(class TableGen* tg) const;
private:
	int getFuncIdx(const class Func* f) const;
	funcidx_map	fidxs;
	funcidx_list	f_list;

	class llvm::GlobalVariable*	memo_table;
};

#endif
