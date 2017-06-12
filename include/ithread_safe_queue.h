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

//ThreadSafeQueue Interface
template<typename T>
class IThreadSafeQueue {

public:

	virtual bool push(const T&) = 0;

	virtual bool push(T&&) = 0;

	virtual bool pop() = 0;

	virtual bool popOnSuccses(const std::function<bool(const T&)>&) = 0;

	virtual void consumeAll(const std::function<void(const T&)>&) = 0;

	virtual bool isEmpty() const = 0;

};

#endif /* ITHREADSAFEQUEUE_H_ */
