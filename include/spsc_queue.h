#ifndef SPSCQUEUE_H_
#define SPSCQUEUE_H_

#include "ithread_safe_queue.h"
#include "cyclic_buffer.h"

template<typename T>
class SpscQueue : public IThreadSafeQueue<T>
{
public:

	SpscQueue(const std::size_t _queue_size)
	:queue(_queue_size)
	{
	}

	SpscQueue(SpscQueue<T>&) = delete;
	SpscQueue(SpscQueue<T>&&) = default;

	bool push(const T& element) override
	{
		return queue.push(element);
	}

	bool push(T&& element) override
	{
		return queue.push(std::move(element));
	}

	bool pop() override
	{
		return queue.pop();
	}

	bool popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		return queue.popOnSuccses(function);
	}

	void consumeAll(const std::function<void(const T&)>& function)
	{
		ConsumeAll(function);
	}

	template<typename Functor>
	void ConsumeAll(const Functor& function)
	{
		queue.ConsumeAll(function);
	}

	const size_t getSize() const override
	{
		return queue.getSize();
	}

	CyclicBuffer<T> queue;

};

#endif
