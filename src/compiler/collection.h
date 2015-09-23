#ifndef COLLECTION_H
#define COLLECTION_H

#include <assert.h>
#include <vector>
#include <list>
#include <memory>

template <typename T>
class PtrList : public std::list<std::unique_ptr<T>>
{
public:
	PtrList() {}

	PtrList(const PtrList<T> &p)
		: std::list<std::unique_ptr<T>>()
	{
		for (auto &t : p) this->add(t->copy());
	}

	PtrList(PtrList<T> &&) = delete;

	virtual ~PtrList() {}

	void add(T* t) {
		assert (t != NULL);
		this->push_back(std::unique_ptr<T>(t));
	}

	void clear_nofree() {
		for (auto &t : *this) t.release();
		this->clear();
	}

	std::list<const T*> to_list(void) const {
		std::list<const T*> ret;
		for (const auto& t : *this)
			ret.push_back(t.get());
		return ret;
	}
};

template <typename T> using PtrVec = std::list<std::unique_ptr<T>>;

#endif
