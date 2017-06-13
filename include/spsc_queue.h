#ifndef SPSCQUEUE_H_
#define SPSCQUEUE_H_

#include <array>
#include <atomic>
#include "ithread_safe_queue.h"
#include <boost/lockfree/detail/branch_hints.hpp>

#define mem_relaxed std::memory_order_relaxed
#define mem_acquire std::memory_order_acquire
#define mem_release std::memory_order_release

using boost::lockfree::detail::unlikely;
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
		const auto current_position = writer_position.load(mem_relaxed);
		const auto next_pos = calculateNextPosition(current_position);
		const auto element2(element);
		if(isFullWriter())
		{
			std::this_thread::yield();
			return false;
		}
		*queue[current_position] = std::move(element2);
		writer_position.store(next_pos, mem_release);
		return true;
	}

	bool push(T&& element) override
	{
		const auto current_position = writer_position.load(mem_relaxed);
		auto next_pos = calculateNextPosition(current_position);
		const auto element2(std::move(element));
		if(isFullWriter())
		{
			std::this_thread::yield();
			return false;
		}
		*queue[current_position] = std::move(element2);
		writer_position.store(next_pos, mem_release);
		return true;
	}

	bool pop() override
	{
		auto const next_pos = calculateNextPosition(reader_position.load(mem_relaxed));
		if(isEmptyReader())
		{
			std::this_thread::yield();	
			return false;
		}
		reader_position.store(next_pos, mem_release);
		return true;
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		const auto current_position = reader_position.load(mem_relaxed);
		bool result = function(*queue[current_position]);
		auto const next_pos = calculateNextPosition(current_position);
		if(isEmptyReader() | !result)
			return false;
		reader_position.store(next_pos, mem_release);
		return true;
	}

	void consumeAll(const std::function<void(const T&)>& function)
	{
		ConsumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		auto current_size = getSize();
		while(current_size > 0)
		{
			ConsumeOne(function);
			--current_size;
		}
	}

	template<typename Functor>
	void ConsumeOne(const Functor& function)
	{
		const auto current_position = reader_position.load(mem_relaxed);
		const auto next_pos = calculateNextPosition(current_position);
		function(*queue[current_position]);
		reader_position.store(next_pos, mem_release);
	}

	const size_t getSize() const override
	{
		const auto writer_value = writer_position.load(mem_relaxed);
		const auto reader_value = reader_position.load(mem_relaxed);
		return calculateSize(writer_value, reader_value);
	}

private:

	void initalizeQueue()
	{
		for (int i = 0; i < queue_size; ++i)
			queue[i] = new T();
	}

	const std::size_t calculateNextPosition(const std::size_t& current_position) const
	{
		size_t ret = current_position + 1;
		if(unlikely(ret == queue_size))
			return 0;
		return ret;
	}

	bool isEmptyReader() const
	{
		return unlikely(getSize() == 0);
	}

	bool isFullWriter() const
	{
		return unlikely(getSize() == queue_size - 1);
	}

	const size_t calculateSize(const size_t& writer, const size_t& reader) const
	{
		const size_t ret(writer - reader + queue_size);
		if(writer >= reader)
			return ret - queue_size;
		return ret;
	}

	std::array<T*, queue_size> queue;
	std::atomic<size_t> reader_position;
	std::atomic<size_t> writer_position;
};

#endif
