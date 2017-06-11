/*
 * IThreadSafeQueue.h
 *
 *  Created on: May 30, 2017
 *      Author: daniel
 */

#ifndef ITHREADSAFEQUEUE_H_
#define ITHREADSAFEQUEUE_H_

#include <functional>
#include <memory>
#include "ithread_safe_queue_iterator.h"

//ThreadSafeQueue Interface
template<typename T>
class IThreadSafeQueue {

public:

	virtual void push(const T&) = 0;

	virtual void push(T&&) = 0;

	virtual void pop() = 0;

	virtual void popOnSuccses(const std::function<bool(const T&)>&) = 0;

	//the iterator takes ownership over the queue, which means after this operation the queue will remain empty.
	virtual std::unique_ptr<IThreadSafeQueueIterator<T>> popElements() = 0;

	virtual void consumeAll(const std::function<void(const T&)>&) = 0;

	virtual bool isEmpty() const = 0;
};

#endif /* ITHREADSAFEQUEUE_H_ */
