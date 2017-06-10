/*
 * ithread_safe_queue_iterator.h
 *
 *  Created on: May 30, 2017
 *      Author: daniel
 */

#ifndef ITHREAD_SAFE_QUEUE_ITERATOR_H_
#define ITHREAD_SAFE_QUEUE_ITERATOR_H_

//ThreadSafeIterator Interface
template<typename T>
class IThreadSafeQueueIterator
{

public:

	virtual const T& operator *() const = 0;

	virtual IThreadSafeQueueIterator<T>& operator++ () = 0;

	virtual bool end() const = 0;
};



#endif /* ITHREAD_SAFE_QUEUE_ITERATOR_H_ */
