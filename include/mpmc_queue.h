
#ifndef MPMCQUEUE_H_
#define MPMCQUEUE_H_

#include "ithread_safe_queue.h"
#include <atomic>
#include <array>
#include <boost/lockfree/detail/branch_hints.hpp>


#define MEM_ACQUIRE std::memory_order_acquire
#define MEM_RELEASE std::memory_order_release
#define MEM_RELAXED std::memory_order_relaxed

using boost::lockfree::detail::likely;
using boost::lockfree::detail::unlikely;
using size_t = std::size_t;

template<typename T, const size_t queue_size>
class MpmcQueue : public IThreadSafeQueue<T>
{

public:
	MpmcQueue()
	: writer_position(0)
	, reader_position(0)
	{
		initalizeQueue();
	}

	MpmcQueue(const MpmcQueue&) = delete;
	MpmcQueue(MpmcQueue&&) = default;

	~MpmcQueue()
	{
		for(int i = 0; i < queue_size; ++i)
			delete queue[i];		
	}

	bool push(const T& element) override
	{
		auto current_position = writer_position.load(MEM_RELAXED);
		auto next_position = calculateNext(current_position);
		auto * current_element = queue[current_position];	
		if(!canWrite(current_position))
			return false;

		while(!writer_position.compare_exchange_weak(current_position, next_position, MEM_RELEASE, MEM_RELAXED))
		{	
			next_position = calculateNext(current_position);
			current_element = queue[current_position];
			if(!canWrite(current_position))
				return false;
		}
		new (current_element) T(element);
		return true;
	}

	bool push(T&& element) override
	{
		auto current_position = writer_position.load(MEM_RELAXED);
                auto next_position = calculateNext(current_position);
		auto* current_element = queue[current_position];
                if(!canWrite(current_position))
                        return false;

                while(!writer_position.compare_exchange_weak(current_position, next_position, MEM_RELEASE, MEM_RELAXED))
                {
			next_position = calculateNext(current_position);
			current_element = queue[current_position];
                	if(!canWrite(current_position))
				return false;
		}	
		new (current_element)  T(std::move(element));
		return true;

	}

	bool pop() override
	{
		auto current_position = reader_position.load(MEM_RELAXED);
                auto next_position = calculateNext(current_position);
		auto& current_element = *queue[current_position];
		if(!canRead(current_position))
			return false;

		while(!reader_position.compare_exchange_weak(current_position, next_position, MEM_RELEASE, MEM_RELAXED))
		{
                        next_position = calculateNext(current_position);
			current_element = *queue[current_position];
		}
		current_element.~T();
		return true;
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		return true;
	}

	void consumeAll(const std::function<void(const T&)>& function) override
	{
		//ConsumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		auto current_position = reader_position.load(MEM_RELAXED);
		for(auto current_size = getSizeReader(current_position); current_size > 0; current_size = getSizeReader(current_position))
                {
			auto next_position = calculateNext(current_position);
			auto& current_element = *queue[current_position];
                        while(!reader_position.compare_exchange_weak(current_position, next_position, MEM_RELEASE, MEM_RELAXED))
			{
				next_position = calculateNext(current_position);
                                current_element = *queue[current_position];
				if(!canRead(current_position))
					return;	
			}
			function(current_element, current_position);
			current_element.~T();
		}
	}

	const size_t getSize() const
	{
		const auto writer_pos = writer_position.load(MEM_ACQUIRE);
		const auto reader_pos = reader_position.load(MEM_ACQUIRE);
		return calculateSize(writer_pos, reader_pos);
	}

private:

	void initalizeQueue()
	{
		for(int i = 0; i < queue_size; ++i)
			queue[i] = new T();
	}

	bool canRead(const size_t current_position_reader) const
        {
                return !unlikely(getSizeReader(current_position_reader) == 0);
        }

        bool canWrite(const size_t current_position_writer) const
        {
                return !unlikely(getSizeWriter(current_position_writer) == queue_size - 1);
        }


	const size_t calculateNext(const size_t current_position) const
	{
		size_t ret = current_position + 1;
                if(unlikely(ret == queue_size))
                        return 0;
                return ret;
	}
	
	const size_t calculateSize(const size_t writer, const size_t reader) const
        {
                const size_t ret(writer - reader + queue_size);
                if(writer >= reader)
                        return ret - queue_size;
                return ret;
        }

	const size_t getSizeReader(const size_t reader) const
	{
		const auto writer = writer_position.load(MEM_ACQUIRE);
		return calculateSize(writer, reader);
	}
	
	const size_t getSizeWriter(const size_t writer) const
	{
		const auto reader = reader_position.load(MEM_ACQUIRE);
		return calculateSize(writer, reader);
	}

	std::atomic<size_t> writer_position;
	std::atomic<size_t> reader_position;
	T* queue[queue_size];
};

#endif
