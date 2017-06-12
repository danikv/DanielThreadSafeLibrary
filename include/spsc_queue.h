#ifndef SPSCQUEUE_H_
#define SPSCQUEUE_H_

#include <array>
#include <atomic>
#include "ithread_safe_queue.h"
#include <boost/lockfree/detail/branch_hints.hpp>

#define mem_relaxed std::memory_order_relaxed

using boost::lockfree::detail::unlikely;

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
		const auto next = calculateNextPosition(current_position);
		*queue[current_position] = element;
		if(isFullWriter())
			return false;
		writer_position.store(next, mem_relaxed);
		return true;
	}

	bool push(T&& element) override
	{
		const auto current_position = writer_position.load(mem_relaxed);
                auto next = calculateNextPosition(current_position);
                *queue[current_position] = element;
                if(isFullWriter())
                        return false;
                writer_position.store(next, mem_relaxed);
		return true;
	}

	bool pop() override
	{
		auto const next = calculateNextPosition(reader_position.load(mem_relaxed));
		if(isEmptyReader())
			return false;
		reader_position.store(next, mem_relaxed);
		return true;
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		const auto current_position = reader_position.load(mem_relaxed);
		bool result = function(*queue[current_position]);
		auto const next = calculateNextPosition(current_position);
		if(isEmptyReader())
			return false;
		if(result)
			reader_position.store(next, mem_relaxed);
		return true;
	}

	void consumeAll(const std::function<void(const T&)>& function)
	{
		while(!isEmptyReader())
		{
			auto current_size = getSizeReader();
			while(current_size > 0)
			{
				const auto current_position = reader_position.load(mem_relaxed);
				function(*queue[current_position]);
				const auto next = calculateNextPosition(current_position);
				reader_position.store(next, mem_relaxed);
				--current_size;
			}
		}
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		static std::size_t size = 0;
	        while(!isEmptyReader())
                {
			auto current_size = getSizeReader();
			size += current_size;
			std::cout << "size : " << size << std::endl;
			while(current_size > 0)
			{
                        	const auto current_position = reader_position.load(mem_relaxed);
                        	function(*queue[current_position]);
                        	const auto next = calculateNextPosition(current_position);
                        	reader_position.store(next, mem_relaxed);
				--current_size;
                	}
		}

	}

	bool isEmpty() const override
	{
                const auto writer_value = writer_position.load(std::memory_order_acquire);
                const auto reader_value = reader_position.load(std::memory_order_acquire);
                if(reader_value == writer_value)
                        return false;
                return true;
	}

private:

	void initalizeQueue()
	{
		for (int i = 0; i < queue_size; ++i)
			queue[i] = new T();
	}

	const std::size_t calculateNextPosition(const std::size_t& current_position) const
	{
		std::size_t ret = current_position + 1;
		if(unlikely(ret == queue_size))
			return 0;
		return ret;
	}

	bool isEmptyReader() const
	{
                if(unlikely(getSizeReader() == 0))
                        return true;
                return false;

	}

	bool isFullWriter() const
	{
		const auto writer_value = writer_position.load(mem_relaxed);
		const auto reader_value = reader_position.load(std::memory_order_acquire) - 1;
		if(unlikely(writer_value == reader_value | reader_value + queue_size == writer_value))
			return true;
		return false;
	}

	size_t getSizeReader() const
	{
		const auto writer_value = writer_position.load(std::memory_order_acquire);
                const auto reader_value = reader_position.load(mem_relaxed);
		std::cout << "writer : " << writer_value << ", reader : " <<
		reader_value << std::endl;
		const std::size_t ret(reader_value - writer_value + queue_size);
		if(writer_value > reader_value)
			return ret;
		return ret - queue_size;
	}

	std::array<T*, queue_size> queue;
	std::atomic<std::size_t> reader_position;
	std::atomic<std::size_t> writer_position;
};

#endif
