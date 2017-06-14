#ifndef GROWINGSPSCQUEUE_H_
#define GROWINGSPSCQUEUE_H_

#include "ithread_safe_queue.h"
#include "spsc_queue.h"
#include <mutex>

using size_t = std::size_t;
#define DEFAULT_QUEUE_SIZE 1024

template<typename T>
class GrowingSpscQueue : public IThreadSafeQueue<T>
{
public:
	GrowingSpscQueue()
	: allocatedBlocks(1)
	{
		initalizeQueue();
	}

	GrowingSpscQueue(GrowingSpscQueue<T>&) = delete;
	GrowingSpscQueue(GrowingSpscQueue<T>&&) = default;

	~GrowingSpscQueue()
	{
		delete queue;
	}

	bool push(const T& element) override
	{
		while(!queue->push(element))
			allocateMoreSize();
		return true;
	}
	
	bool push(T&& element) override
	{
		while(!queue->push(std::move(element)))
			allocateMoreSize();
		return true;
	}

	bool pop() override //add compare exchange strong here
	{
		std::unique_lock<std::mutex> locker(mutex);
		return queue->pop();
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		std::unique_lock<std::mutex> locker(mutex);
		return queue->popOnSuccses(function);
	}
	
	void consumeAll(const std::function<void(const T&)>& function) override
	{
		consumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		std::unique_lock<std::mutex> locker(mutex);
		queue->ConsumeAll(function);
	}

	const size_t getSize() const override
	{
		return queue->getSize();
	}

private:

	void initalizeQueue()
	{
		queue = new SpscQueue<T>(DEFAULT_QUEUE_SIZE);
	}

	void allocateMoreSize()
	{
		std::unique_lock<std::mutex> locker(mutex);
		allocatedBlocks *= 2;
		queue  = new SpscQueue<T>(std::move(*queue), capacity());
	}

	const size_t capacity() const
	{
		return allocatedBlocks * DEFAULT_QUEUE_SIZE;
	}

	int allocatedBlocks;
	SpscQueue<T> * queue;
	std::mutex mutex;
};

#endif
