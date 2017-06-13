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

template<typename T, const std::size_t queue_size>
class SpscQueue : public IThreadSafeQueue<T>
{
public:

	SpscQueue()
	: writer_position(0)
	, reader_position(0)
	{
		initalizeQueue();
	}

	SpscQueue(SpscQueue<T,queue_size>&) = delete;
	SpscQueue(SpscQueue<T,queue_size>&&) = default;

	~SpscQueue()
	{
		for (int i = 0; i < queue_size; ++i)
			delete queue[i];
	}

	bool push(const T& element) override
	{
		const auto current_position = writer_position.load(MEM_RELAXED);
		const auto next_pos = calculateNextPosition(current_position, queue_size);
		if(!canWrite(current_position))
			return false;
		new (queue[current_position]) T(element);
		writer_position.store(next_pos, MEM_RELEASE);
		return true;
	}

	bool push(T&& element) override
	{
		const auto current_position = writer_position.load(MEM_RELAXED);
		auto next_pos = calculateNextPosition(current_position, queue_size);
		if(!canWrite(current_position))
			return false;
		new (queue[current_position]) T(std::move(element));
		writer_position.store(next_pos, MEM_RELEASE);
		return true;
	}

	bool pop() override
	{
		const auto current_position = reader_position.load(MEM_RELAXED);
		const auto next_pos = calculateNextPosition(current_position, queue_size);
		if(!canRead(current_position))
			return false;
		reader_position.store(next_pos, MEM_RELEASE);
		queue[current_position]->~T();
		return true;
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		const auto current_position = reader_position.load(MEM_RELAXED);
		bool result = function(*queue[current_position]);
		auto const next_pos = calculateNextPosition(current_position, queue_size);
		if(!canRead(current_position) | !result)
			return false;
		reader_position.store(next_pos, MEM_RELEASE);
		queue[current_position]->~T();
		return true;
	}

	void consumeAll(const std::function<void(const T&)>& function)
	{
		ConsumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		auto current_pos = reader_position.load(MEM_RELAXED);
		for(auto current_size = availableRead(current_pos); current_size > 0;
				current_size = availableRead(current_pos))
		{
			const int overflow = current_pos + current_size;
			if(overflow > queue_size)
			{
				consume(function, current_pos, queue_size);
				consume(function, 0, overflow - queue_size);
				current_pos = calculateNextPosition(overflow - queue_size - 1, queue_size);
			}
			else
			{
				consume(function, current_pos, overflow);
				current_pos = calculateNextPosition(overflow - 1, queue_size);
			}
			reader_position.store(current_pos, MEM_RELEASE);
		}
	}

	const size_t getSize() const override
	{
		const auto writer_value = writer_position.load(MEM_ACQUIRE);
		const auto reader_value = reader_position.load(MEM_ACQUIRE);
		return availableRead(writer_value, reader_value, queue_size);
	}

private:

	void initalizeQueue()
	{
		for (int i = 0; i < queue_size; ++i)
			queue[i] = new T();
	}

	bool canRead(const size_t reader_pos) const
	{
		return likely(availableRead(reader_pos) > 0);
	}

	const size_t availableRead(const size_t reader_pos) const
	{
		const auto writer_pos = writer_position.load(MEM_RELAXED);
		return availableRead(reader_pos, writer_pos, queue_size);
	}

	bool canWrite(const size_t writer_pos) const
	{
		const auto reader_pos = reader_position.load(MEM_RELAXED);
		return likely(availableRead(reader_pos, writer_pos, queue_size) < queue_size - 1);
	}

	template<typename Functor>
	void consume(const Functor& function, const size_t start_index, const size_t end_index) const
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

	static const int padding_size = CACHE_LINE_SIZE - sizeof(size_t);

	std::atomic<size_t> reader_position;
	char padding1[padding_size]; /* force read_index and write_index to different cache lines */
	std::atomic<size_t> writer_position;

	T* queue[queue_size];
};

#endif
