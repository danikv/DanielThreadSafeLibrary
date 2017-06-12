#ifndef IBLOCKINGTHREADSAFEQUEUE_H_
#define IBLOCKINGTHREADSAFEQUEUE_H_

#include "ithread_safe_queue.h"

template<typename T>
class IBlockingThreadSafeQueue : public IThreadSafeQueue<T>
{
	
public:
	
	virtual void blockingConsumeAll(const std::function<void(const T&)>&) = 0;
	
	virtual void notifyReaders() = 0;
};


#endif
