#ifndef GROWINGSPSCQUEUE_H_
#define GROWINGSPSCQUEUE_H_

#include "ithread_safe_queue.h"
#include "cyclic_buffer.h"

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
		delete writer_queue;
		if(reader_queue != writer_queue)
			delete reader_queue;
	}

	bool push(const T& element) override
	{
		while(unlikely(!writer_queue->push(element)))
			allocateMoreSize();
		return true;
	}
	
	bool push(T&& element) override
	{
		while(unlikely(!writer_queue->push(std::move(element))))
			allocateMoreSize();
		return true;
	}

	bool pop() override
	{
		auto * queue = last_used_queue.load(MEM_ACQUIRE);
		if(isQueueChanged(queue))
			syncQueue(queue);
		return reader_queue->pop();
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		auto * queue = last_used_queue.load(MEM_ACQUIRE);
		if(isQueueChanged(queue))
			syncQueue(queue);
		return reader_queue->popOnSuccses(function);
	}
	
	void consumeAll(const std::function<void(const T&)>& function) override
	{
		ConsumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		auto * queue = last_used_queue.load(MEM_ACQUIRE);
		if(isQueueChanged(queue))
			syncQueue(queue);
		reader_queue->ConsumeAll(function);
	}

	const size_t getSize() const override
	{
		return 0;
	}

	const size_t capacity() const
	{
		return allocatedBlocks * DEFAULT_QUEUE_SIZE;
	}

private:

	void initalizeQueue()
	{
		writer_queue = new CyclicBuffer<T>(DEFAULT_QUEUE_SIZE);
		reader_queue = writer_queue;
		last_used_queue.store(nullptr, MEM_RELAXED);
	}

	void allocateMoreSize()
	{
		allocatedBlocks *= 2;
		auto * new_queue = new CyclicBuffer<T>(*(writer_queue), capacity());
		if(last_used_queue.compare_exchange_strong(writer_queue, new_queue, MEM_RELEASE, MEM_RELAXED))
			writer_queue->deleteBuffer();
		else
			last_used_queue.store(new_queue, MEM_RELEASE);
		writer_queue = new_queue;
	}

	void syncQueue(CyclicBuffer<T> * queue)
	{
		while(!last_used_queue.compare_exchange_strong(queue, nullptr, MEM_RELEASE, MEM_RELAXED));
		queue->syncReader(*reader_queue);
		reader_queue->deleteBuffer();
		reader_queue = queue;
	}
	
	bool isQueueChanged(const CyclicBuffer<T> * queue) const
	{
		return unlikely(queue != nullptr);
	}

	static const int padding_size = CACHE_LINE_SIZE - sizeof(CyclicBuffer<T> *);

	CyclicBuffer<T> * writer_queue;
	char padding1[padding_size]; /* force writer_queue and reader_queue to different cache lines */
	CyclicBuffer<T> * reader_queue;
	int allocatedBlocks;
	std::atomic<CyclicBuffer<T> *> last_used_queue;
};

#endif
