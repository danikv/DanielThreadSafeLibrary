#ifndef GROWINGSPSCQUEUE_H_
#define GROWINGSPSCQUEUE_H_

#include "ithread_safe_queue.h"
#include "spsc_queue.h"

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
		{
			allocateMoreSize();
		}
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
		if(isRealloc.load(MEM_ACQUIRE))
			return false;
		std::atomic_thread_fence(MEM_ACQUIRE);
		return queue->pop();
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		if(isRealloc.load(MEM_ACQUIRE))
			return false;
		std::atomic_thread_fence(MEM_ACQUIRE);
		return queue->popOnSuccses(function);
	}
	
	void consumeAll(const std::function<void(const T&)>& function) override
	{
		consumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		if(isRealloc.load(MEM_ACQUIRE))
			return;
		std::atomic_thread_fence(MEM_ACQUIRE);
		queue->ConsumeAll(function);
	}


	const size_t getSize() const override
	{
		if(isRealloc.load(MEM_ACQUIRE))
			return 0;
		std::atomic_thread_fence(MEM_ACQUIRE);
		return queue->getSize();
	}

private:

	void initalizeQueue()
	{
		queue = new SpscQueue<T>(DEFAULT_QUEUE_SIZE);
	}

	void allocateMoreSize()
	{
		allocatedBlocks *= 2;
		queue  = new SpscQueue<T>(std::move(*queue), allocatedBlocks *  allocatedBlocks * DEFAULT_QUEUE_SIZE);
		std::atomic_thread_fence(MEM_RELEASE);
		isRealloc.store(true, MEM_RELEASE);
		isRealloc.store(false, MEM_RELAXED);
	}

	void capacity()
	{
		return allocatedBlocks * DEFAULT_QUEUE_SIZE;
	}

	int allocatedBlocks;
	std::atomic<bool> isRealloc;
	SpscQueue<T> * queue;
};

#endif
