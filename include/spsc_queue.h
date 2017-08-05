#ifndef SPSCQUEUE_H_
#define SPSCQUEUE_H_

#include "cyclic_buffer.h"

template <class T>
class SpscQueue
{
public:

	SpscQueue(const std::size_t _queue_size)
	:queue(_queue_size)
	{
	}

	SpscQueue(SpscQueue<T>&) = delete;
	SpscQueue(SpscQueue<T>&&) = default;

	bool push(const T& element)
	{
		return queue.push(element);
	}

	bool push(T&& element)
	{
		return queue.push(std::move(element));
	}

	bool pop()
	{
		return queue.pop();
	}
	
	/*
	 * this function will not throw but it can fail and will return false if so otherwise true and element will contain
	 * a value
	 */
	bool tryPop(T& element)
	{
		return queue.tryPop(element);
	}
	
	//this function will not throw but it can fail and will reutnr nullptr if so , otherwise unique_ptr with the value
	std::unique_ptr<T> tryPop()
	{
		return queue.tryPop();
	}
	
	template<typename Functor>
	bool PopOnSuccses(const Functor& function)
	{
		return queue.popOnSuccses(function);
	}
	
	template<typename Functor>
	void consumeOne(const Functor& function)
	{
		queue.consumeOne(function);
	}

	template<typename Functor>
	void consumeAll(const Functor& function)
	{
		queue.consumeAll(function);
	}
	
	//should be only used by the consumer , returns true when the queue isnt empty otherwise false
	bool canRead() const
	{
		return queue.canRead();
	}

	//should be only used by the producer , returns true when the queue isnt full otherwise false
	bool canWrite() const
	{
		return queue.canWrite();
	}

	CyclicBuffer<T> queue;

};

#endif
