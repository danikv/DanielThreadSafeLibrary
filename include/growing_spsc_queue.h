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
	, isRealloc(false)
	{
		initalizeQueue();
	}

	GrowingSpscQueue(GrowingSpscQueue<T>&) = delete;
	GrowingSpscQueue(GrowingSpscQueue<T>&&) = default;

	~GrowingSpscQueue()
	{
		delete writer_queue;
		if(reader_queue != writer_queue)
			delete reader_queue;
	}

	bool push(const T& element) override
	{
		while(!writer_queue->push(element))
			allocateMoreSize();
		return true;
	}
	
	bool push(T&& element) override
	{
		while(!writer_queue->push(std::move(element)))
			allocateMoreSize();
		return true;
	}

	bool pop() override
	{
		if(isRealloc.load(MEM_ACQUIRE))
			syncQueue();
		return reader_queue->pop();
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		if(isRealloc.load(MEM_ACQUIRE))
			syncQueue();
		return reader_queue->popOnSuccses(function);
	}
	
	void consumeAll(const std::function<void(const T&)>& function) override
	{
		ConsumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		if(isRealloc.load(MEM_RELAXED))
			syncQueue();
		reader_queue->ConsumeAll(function);
	}

	const size_t getSize() const override
	{
		return 0;
	}

private:

	void initalizeQueue()
	{
		writer_queue = new SpscQueue<T>(DEFAULT_QUEUE_SIZE);
		reader_queue = writer_queue;
	}

	void allocateMoreSize()
	{
		allocatedBlocks *= 2;
		writer_queue  = new SpscQueue<T>(*writer_queue, capacity());
		isRealloc.store(true, MEM_RELAXED);
	}

	const size_t capacity() const
	{
		return allocatedBlocks * DEFAULT_QUEUE_SIZE;
	}

	void syncQueue()
	{
		writer_queue->syncReader(*reader_queue);
		reader_queue = writer_queue;
		isRealloc.store(false, MEM_RELAXED);
	}

	friend class SpscQueue<T>;

	static const int padding_size = CACHE_LINE_SIZE - sizeof(SpscQueue<T> *);

	SpscQueue<T> * writer_queue;
	char padding1[padding_size]; /* force reader_position and writer_position to different cache lines */
	SpscQueue<T> * reader_queue;
	std::atomic<bool> isRealloc;
	int allocatedBlocks;
};

#endif
