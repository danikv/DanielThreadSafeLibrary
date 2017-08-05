#ifndef GROWINGSPSCQUEUE_H_
#define GROWINGSPSCQUEUE_H_

#include "cyclic_buffer.h"

#define DEFAULT_QUEUE_SIZE 1024

template <class T>
class GrowingSpscQueue
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

	/*
	 * push function cannot fail , they can only throw exception when there is no more memory to allocate
	 */
	void push(const T& element)
	{
		allocateSizeIfNeeded();
		writer_queue->unSafePush(element);
	}
	
	void push(T&& element)
	{
		allocateSizeIfNeeded();
		writer_queue->unSafePush(std::move(element));
	}

	bool pop()
	{
		syncReaderQueue();
		return reader_queue->pop();
	}
	
	/*
	 * this function will not throw but it can fail and will return false if so otherwise true and element will contain
	 * a value
	 */
	bool tryPop(T& element)
	{
		syncReaderQueue();
		return reader_queue->tryPop(element);
	}
	
	//this function will not throw but it can fail and will reutnr nullptr if so , otherwise unique_ptr with the value
	std::unique_ptr<T> tryPop()
	{
		syncReaderQueue();
		return reader_queue->tryPop();
	}

	template<typename Functor>
	bool popOnSuccses(const Functor& function)
	{
		syncReaderQueue();
		return reader_queue->popOnSuccses(function);
	}

	template<typename Functor>
	void consumeAll(const Functor& function)
	{
		syncReaderQueue();
		reader_queue->consumeAll(function);
	}

	//should be only used by the consumer , returns true when the queue isnt empty otherwise false
	bool canRead() const
	{
		return reader_queue->canRead();
	}

	//should be only used by the producer , returns true when the queue isnt full otherwise false
	bool canWrite() const
	{
		return writer_queue->canWrite();
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
		while(!last_used_queue.compare_exchange_weak(queue, nullptr, MEM_ACQUIRE, MEM_RELAXED));
		queue->syncReader(*reader_queue);
		reader_queue->deleteBuffer();
		reader_queue = queue;
	}
	
	bool isQueueChanged(const CyclicBuffer<T> * queue) const
	{
		return unlikely(queue != nullptr);
	}
	
	void syncReaderQueue()
	{
		auto * queue = last_used_queue.load(MEM_RELAXED);
		if(isQueueChanged(queue))
			syncQueue(queue);
	}
	
	void allocateSizeIfNeeded()
	{
		if(!canWrite())
			allocateMoreSize();
	}

	static const int padding_size = CACHE_LINE_SIZE - sizeof(CyclicBuffer<T> *);

	CyclicBuffer<T> * reader_queue;
	std::atomic<CyclicBuffer<T> *> last_used_queue;
	char padding1[padding_size]; /* force writer_queue and reader_queue to different cache lines */
	CyclicBuffer<T> * writer_queue;
	int allocatedBlocks;
};

#endif
