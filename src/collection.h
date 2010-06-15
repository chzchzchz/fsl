#ifndef COLLECTION_H
#define COLLECTION_H

#include <assert.h>

#include <vector>
#include <list>

template <typename T>
class PtrList : public std::list<T*> 
{
public:	
	PtrList() {}
	virtual ~PtrList() 
	{
		typename std::list<T*>::iterator	it;
		for (it = this->begin(); it != this->end(); it++) {
			delete (*it);
		}
	}
	void add(T* t) { assert (t != NULL); push_back(t); }
};

#endif
