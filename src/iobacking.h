/**
 * Interface to describe how data in physical types is stored/loaded etc
 * at a bit level.
 */
#ifndef IOBACKING_H
#define IOBACKING_H

#include <stdint.h>
#include "collection.h"

/**
 * An IOData represents a single 'chunk' of data being requested.
 */
class IOData
{
public:
protected:
	/* should only be initialized by an IOBacking */
	IOData() {}
};

/**
 * backing structure (e.g. file, disk, ...)
 */
class IOBacking
{
public:
	virtual ~IOBacking() {}
	
	IOData* get(uint64_t byte_off, uint32_t length);


/* XXX figure this out later--- possibly use featherstitch-style techniques? */
#if 0
	/** all puts after barrier are commited after all puts before
	 * barrier have committed */
	virtual void putBarrier(IOData*) = 0;
	virtual void putBarrier(const PtrList<IOData*>& iod) = 0;

	/**
	 * these writes may be reordered in stream at will
	 */
	virtual void put(IOData*) = 0;
	virtual void put(const PtrList<IOData*>& iod) = 0;


	/**
	 * data must be written ordered in the way given 
	 * (but may be merged if appropriate)
	 */
	virtual void putOrdered(const PtrList<IOData*>& iod) = 0;
#endif
	virtual unsigned int getGranularity() const;
	
protected:
	IOBacking() {}

};
#endif
