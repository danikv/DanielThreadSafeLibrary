#ifndef SPSCQUEUE_H_
#define SPSCQUEUE_H_

#include <array>
#include <atomic>
#include "ithread_safe_queue.h"
#include <boost/lockfree/detail/branch_hints.hpp>

#define MEM_RELAXED std::memory_order_relaxed
#define MEM_ACQUIRE std::memory_order_acquire
#define MEM_RELEASE std::memory_order_release
#define CACHE_LINE_SIZE 64

using boost::lockfree::detail::unlikely;
using boost::lockfree::detail::likely;
using size_t = std::size_t;

template<typename T>
class SpscQueue : public IThreadSafeQueue<T>
{
public:

	SpscQueue(const std::size_t _queue_size)
	: writer_position(0)
	, reader_position(0)
	, queue_size(_queue_size)
	{
		initalizeQueue();
	}

	SpscQueue(SpscQueue<T>&) = delete;
	SpscQueue(SpscQueue<T>&&) = default;

	//use to initalize to queue with diffrent queue size(used by growing_spsc_queue)
	SpscQueue(const SpscQueue<T>& _queue, const size_t _queue_size)
	: writer_position(_queue.queue_size - 1)
	, reader_position(0)
	, queue_size(_queue_size)
	{
		queue = new T*[queue_size];
		size_t reader_pos = reader_position.load(MEM_ACQUIRE);
		for(int i = 0; i < _queue.queue_size; ++i)
		{
			queue[i] = _queue.queue[reader_pos];
			reader_pos = calculateNextPosition(reader_pos, _queue.queue_size);
		}
		for(int i = _queue.queue_size; i < queue_size; ++i)
			queue[i] = new T();
	}

	~SpscQueue()
	{
		if(queue != nullptr)
		{
			for (int i = 0; i < queue_size; ++i)
				delete queue[i];
		}
	}

	bool push(const T& element) override
	{
		const size_t current_position = writer_position.load(MEM_RELAXED);
		const size_t next_pos = calculateNextPosition(current_position, queue_size);
		if(!canWrite(current_position))
			return false;
		new (queue[current_position]) T(element);
		writer_position.store(next_pos, MEM_RELEASE);
		return true;
	}

	bool push(T&& element) override
	{
		const size_t current_position = writer_position.load(MEM_RELAXED);
		const size_t next_pos = calculateNextPosition(current_position, queue_size);
		if(!canWrite(current_position))
			return false;
		new (queue[current_position]) T(std::move(element));
		writer_position.store(next_pos, MEM_RELEASE);
		return true;
	}

	bool pop() override
	{
		const size_t current_position = reader_position.load(MEM_RELAXED);
		const size_t next_pos = calculateNextPosition(current_position, queue_size);
		if(!canRead(current_position))
			return false;
		increaseReaderPos(current_position, next_pos);
		return true;
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		const size_t current_position = reader_position.load(MEM_RELAXED);
		bool result = function(*queue[current_position]);
		const size_t next_pos = calculateNextPosition(current_position, queue_size);
		if(!canRead(current_position) | !result)
			return false;
		increaseReaderPos(current_position, next_pos);
		return true;
	}

	void consumeAll(const std::function<void(const T&)>& function)
	{
		ConsumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		size_t current_pos = reader_position.load(MEM_RELAXED);
		for(size_t current_size = availableRead(current_pos); current_size > 0;
				current_size = availableRead(current_pos))
		{
			current_pos = consumeSize(function, current_pos, current_size);
			reader_position.store(current_pos, MEM_RELEASE);
		}
	}

	const size_t getSize() const override
	{
		const size_t writer_value = writer_position.load(MEM_ACQUIRE);
		const size_t reader_value = reader_position.load(MEM_ACQUIRE);
		return availableRead(reader_value, writer_value, queue_size);
	}

	void syncReader(const SpscQueue<T>& _queue)
	{
		reader_position.store((_queue.queue_size - 1) - _queue.getSize(), MEM_RELEASE);
		delete [] _queue.queue;
	}

private:

	void initalizeQueue()
	{
		queue = new T*[queue_size];
		for (int i = 0; i < queue_size; ++i)
			queue[i] = new T();
	}

	bool canRead(const size_t reader_pos) const
	{
		return likely(availableRead(reader_pos) > 0);
	}

	const size_t availableRead(const size_t reader_pos) const
	{
		const size_t writer_pos = writer_position.load(MEM_ACQUIRE);
		return availableRead(reader_pos, writer_pos, queue_size);
	}

	bool canWrite(const size_t writer_pos) const
	{
		const size_t reader_pos = reader_position.load(MEM_ACQUIRE);
		return likely(availableRead(reader_pos, writer_pos, queue_size) < queue_size - 1);
	}

	template<typename Functor>
	const size_t consumeSize(const Functor& function, const size_t current_pos, const size_t current_size) const
	{
		const size_t end_index = current_pos + current_size;
		if(end_index > queue_size)
		{
			consumeRange(function, current_pos, queue_size);
			consumeRange(function, 0, end_index - queue_size);
			return calculateNextPosition(end_index - queue_size - 1, queue_size);
		}
		else
		{
			consumeRange(function, current_pos, end_index);
			return calculateNextPosition(end_index - 1, queue_size);
		}
	}

	template<typename Functor>
	void consumeRange(const Functor& function, const size_t start_index, const size_t end_index) const
	{
		for(int i = start_index; i < end_index; ++i)
		{
			function(*queue[i]);
			queue[i]->~T();
		}
	}

	static const size_t availableRead(const size_t reader_pos, const size_t writer_pos, const size_t size)
	{
		if(writer_pos >= reader_pos)
			return writer_pos - reader_pos;
		return writer_pos - reader_pos + size;
	}

	static const std::size_t calculateNextPosition(const size_t current_position, const size_t size)
	{
		size_t ret = current_position + 1;
		if(unlikely(ret == size))
			return 0;
		return ret;
	}

	void increaseReaderPos(const size_t current_position, const size_t next_position)
	{
		reader_position.store(next_position, MEM_RELEASE);
		queue[current_position]->~T();
	}

	T** queue;

	static const int padding_size = CACHE_LINE_SIZE - sizeof(size_t);

	std::atomic<size_t> reader_position;
	char padding1[padding_size]; /* force reader_position and writer_position to different cache lines */
	std::atomic<size_t> writer_position;
	const size_t queue_size;

};

#endif
