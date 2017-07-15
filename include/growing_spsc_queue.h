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
		while(unlikely(!writer_queue->current->push(element)))
			allocateMoreSize();
		return true;
	}
	
	bool push(T&& element) override
	{
		while(unlikely(!writer_queue->current->push(std::move(element))))
			allocateMoreSize();
		return true;
	}

	bool pop() override
	{
		if(isQueueChanged())
			syncQueue();
		return reader_queue->current->pop();
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		if(isQueueChanged())
			syncQueue();
		return reader_queue->current->popOnSuccses(function);
	}
	
	void consumeAll(const std::function<void(const T&)>& function) override
	{
		ConsumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		if(isQueueChanged())
			syncQueue();
		reader_queue->current->ConsumeAll(function);
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
	

	struct QueueBlock
	{
		QueueBlock(const size_t queue_size)
		: current(new CyclicBuffer<T>(queue_size))
		, next(nullptr)
		{
		}
		
		QueueBlock(const CyclicBuffer<T>& queue, const size_t queue_size)
		: current(new CyclicBuffer<T>(queue, queue_size))
		, next(nullptr)
		{
		}
		
		~QueueBlock()
		{
			if(current != nullptr)
				current->deleteBuffer();
		}

		CyclicBuffer<T> * current;
		std::atomic<QueueBlock *> next;
	};

	void initalizeQueue()
	{
		writer_queue = new QueueBlock(DEFAULT_QUEUE_SIZE);
		reader_queue = writer_queue;
	}

	void allocateMoreSize()
	{
		allocatedBlocks *= 2;
		auto * next_queue = new QueueBlock(*(writer_queue->current), capacity());
		writer_queue->next.store(next_queue, MEM_RELEASE);
		writer_queue = next_queue;
	}

	void syncQueue()
	{
		const auto * current_reader = reader_queue;
		reader_queue = reader_queue->next;
		while(reader_queue->next.load(MEM_ACQUIRE) != nullptr)
			advanceQueue(&reader_queue);
		reader_queue->current->syncReader(*(current_reader->current));
		delete current_reader;
	}

	bool isQueueChanged() const
	{
		return unlikely(reader_queue->next.load(MEM_ACQUIRE) != nullptr);
	}

	static void advanceQueue(QueueBlock** const queue)
	{
		const auto * current = *queue;
		*queue = (*queue)->next;
		delete current;
	}

	static const int padding_size = CACHE_LINE_SIZE - sizeof(QueueBlock *);

	QueueBlock * writer_queue;
	char padding1[padding_size]; /* force writer_queue and reader_queue to different cache lines */
	QueueBlock * reader_queue;
	int allocatedBlocks;
};

#endif
