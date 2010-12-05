#ifndef RELOC_H
#define RELOC_H

#include "table_gen.h"
#include "instance_iter.h"

typedef PtrList<class RelocTypes>			typereloc_list;
typedef std::map<std::string, class RelocTypes*>	typereloc_map;
typedef PtrList<class Reloc>				reloc_list;

class WritePktInstance;

/* relocs for a type */
class RelocTypes : TableWriter
{
public:
	RelocTypes(const class Type* t);
	virtual ~RelocTypes() {}

	void genProtos(void);
	void genCode(void);
	virtual void genExterns(TableGen* tg);
	virtual void genTables(TableGen* tg);

	const Type* getType() const { return src_type; }
private:
	class Reloc* loadReloc(const class Preamble* p);

	RelocTypes() {}
	const Type	*src_type;
	unsigned int	seq;
	reloc_list	relocs;
};

/* single reloc */
class Reloc
{
public:
	Reloc(	RelocTypes* parent,
		unsigned int seq,
		const Id* as_name,
		InstanceIter*	in_sel_iter,
		InstanceIter*	in_choice_iter,
		CondExpr*	in_choice_cond,
		WritePktInstance	*in_wpkt_alloc,
		WritePktInstance	*in_wpkt_relink,
		WritePktInstance	*in_wpkt_replace
		);
	virtual ~Reloc();

	void genCode(void) const;
	void genProtos(void) const;
	void genExterns(TableGen*) const;
	void genTableInstance(TableGen*) const;
	const WritePktInstance* getAlloc(void) const { return wpkt_alloc; }
	const WritePktInstance* getRelink(void) const { return wpkt_relink; }
	const WritePktInstance* getReplace(void) const { return wpkt_replace; }
	const Id* getName(void) const { return as_name; }
private:
	void genCondProto(void) const;
	void genCondCode(void) const;
	std::string getCondFuncName(void) const;

	RelocTypes*		parent;
	unsigned int		seq;
	InstanceIter*		sel_iter;
	InstanceIter*		choice_iter;
	CondExpr*		choice_cond;
	WritePktInstance*	wpkt_alloc;
	WritePktInstance*	wpkt_relink;
	WritePktInstance*	wpkt_replace;
	Id*			as_name;
};

#endif
