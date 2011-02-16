#ifndef RELOC_H
#define RELOC_H

#include "annotation.h"
#include "table_gen.h"
#include "instance_iter.h"

typedef PtrList<class RelocTypes>			typereloc_list;
typedef std::map<std::string, class RelocTypes*>	typereloc_map;
typedef PtrList<class Reloc>				reloc_list;

class WritePktInstance;

/* relocs for a type */
class RelocTypes : TableWriter, public Annotation
{
public:
	RelocTypes(const class Type* t);
	virtual ~RelocTypes() {}

	virtual void genExterns(TableGen* tg);
	virtual void genTables(TableGen* tg);

	unsigned int getNumRelocs(void) const { return relocs.size(); }
	virtual const anno_ent_list* getEntryList(void) const
		{ return (const anno_ent_list*)&relocs; }

protected:
	virtual void load(const Preamble* p);
private:
	class Reloc* loadReloc(const class Preamble* p);
	reloc_list	relocs;
};

/* single reloc */
class Reloc : public AnnotationEntry
{
public:
	Reloc(	RelocTypes*	parent,
		const Preamble* pre,
		InstanceIter*	in_sel_iter,
		InstanceIter*	in_choice_iter,
		CondExpr*	in_choice_cond,
		WritePktInstance	*in_wpkt_alloc,
		WritePktInstance	*in_wpkt_relink,
		WritePktInstance	*in_wpkt_replace);
	virtual ~Reloc();

	void genCode(void);
	void genProto(void);

	void genExterns(TableGen*) const;
	void genTableInstance(TableGen*) const;
	const WritePktInstance* getAlloc(void) const { return wpkt_alloc; }
	const WritePktInstance* getRelink(void) const { return wpkt_relink; }
	const WritePktInstance* getReplace(void) const { return wpkt_replace; }

private:
	void genCondProto(void) const;
	void genCondCode(void) const;
	std::string getCondFuncName(void) const;

	InstanceIter*		sel_iter;
	InstanceIter*		choice_iter;
	CondExpr*		choice_cond;
	WritePktInstance*	wpkt_alloc;
	WritePktInstance*	wpkt_relink;
	WritePktInstance*	wpkt_replace;
};

#endif
