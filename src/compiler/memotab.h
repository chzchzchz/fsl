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
	typedef std::list<const class Func*>		funcidx_list;
	MemoTab(void) : last_datatab_idx(0), memo_datatab(NULL) {}
	virtual ~MemoTab(void) {}
	void registerFunction(const class Func* f);
	void allocTable(void);
	class llvm::Value* memoFuncCall(const class Func* f) const;
	bool canMemoize(const class Func* f) const;
	void genTables(class TableGen* tg) const;
private:
	int getFuncIdx(const class Func* f) const;
	unsigned int getNumTabEntries(const class Func* f) const;
	class llvm::Value* memoCallPrim(const class Func* f) const;
	class llvm::Value* memoCallType(const class Func* f) const;
	unsigned int	last_datatab_idx;
	funcidx_map	datatab_idxs;
	funcidx_list	f_list;

	class llvm::GlobalVariable*	memo_datatab;
};

#endif
