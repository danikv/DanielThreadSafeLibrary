#pragma once


#include <array>
#include <atomic>
#include "queue_exceptions.h"
#include "thread_safe_queue_iterator.h"
#include "ithread_safe_queue.h"

template<typename T, const int queue_size>
class SpsqQueue : public IThreadSafeQueue<T>
{
public:

	SpsqQueue()
	: writer_position(0)
	, reader_position(0)
	, current_queue_size(0)
	{
		initalizeQueue();
	}

	SpsqQueue(SpsqQueue<T,queue_size>&) = delete;
	SpsqQueue(SpsqQueue<T,queue_size>&&) = default;

	~SpsqQueue()
	{
		for (int i = 0; i < queue_size; ++i)
			delete queue[i];
	}

	void push(const T& element) override
	{
		throwIfFull();
		*writer_position = element;
		writer_position = calculateNextPosition(writer_position);
		current_queue_size++;
	}

	void pop() override
	{
		throwIfEmpty();
		unsafePop();
	}

	void popOnSuccses(std::function<bool(const T&)> function) override
	{
		throwIfEmpty();
		if(function(*reader_position))
			unsafePop();
	}

	std::unique_ptr<IThreadSafeQueueIterator<T>> popElements() override
	{
		const auto current_position = [&]() {
			return reader_position;
		};
		const auto calculate_next_position = [&](){
			unsafePop();
		};
		const auto end_function = [&](const T* end_index) {
			return  reader_position == end_index;
		};
		return std::make_unique<ThreadSafeQueueIterator<T>>(std::bind(end_function, writer_position),
				calculate_next_position, current_position);
	}

	bool isEmpty() const override
	{
		return getSize() == 0;
	}

private:

	void initalizeQueue()
	{
		for (int i = 0; i < queue_size; ++i)
			queue[i] = new T();
		writer_position = reader_position = queue.front();
	}

	T* calculateNextPosition(T* current_position)
	{
		if(current_position == queue.back())
			current_position = queue.front();
		else
			current_position++;
		return current_position;
	}

	void throwIfEmpty() const
	{
		if(isEmpty())
			throw QueueEmpty();
	}

	void throwIfFull() const
	{
		if(getSize() == queue_size)
			throw QueueFull();
	}

	const int getSize() const
	{
		return current_queue_size.load(std::memory_order_acquire);
	}

	void unsafePop()
	{
		reader_position = calculateNextPosition(reader_position);
		current_queue_size--;
	}

	std::array<T*, queue_size> queue;
	T* writer_position;
	T* reader_position;
	std::atomic<int> current_queue_size;
};
