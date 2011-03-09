#ifndef FSLSTAT_H
#define FSLSTAT_H

#include "collection.h"
#include "table_gen.h"
#include "annotation.h"

typedef PtrList<class StatEnt>			statent_list;
typedef std::map<std::string, class Stat*>	stat_map;
typedef std::list<class Stat*>			stat_list;

class Stat : public Annotation, public TableWriter
{
public:
	Stat(const Type* t);
	virtual ~Stat(void);

	unsigned int getNumStat(void) const { return stat_elems.size(); }
	virtual const anno_ent_list* getEntryList(void) const
		{ return (const anno_ent_list*)(&stat_elems); }

	virtual void genExterns(TableGen* tg);
	virtual void genTables(TableGen* tg);
protected:
	virtual void load(const Preamble* p);
private:
	statent_list	stat_elems;
};

class StatEnt : public AnnotationEntry
{
public:
	StatEnt(Stat* in_parent, const Preamble* in_pre);
	virtual ~StatEnt(void) {}

	void genCode(void);
	void genProto(void);
	void genInstance(TableGen* tg) const;

private:
	StatEnt() {}
};


#endif