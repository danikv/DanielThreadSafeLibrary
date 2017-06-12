#ifndef BLOCKINGTHREADSAFEQUEUE_H_
#define BLOCKINGTHREADSAFEQUEUE_H_

#include <condition_variable>
#include <mutex>
#include <atomic>
#include "iblocking_thread_safe_queue.h"

template<typename T, typename QueueType>
class BlockingThreadSafeQueue : public IBlockingThreadSafeQueue<T>
{

public:
	BlockingThreadSafeQueue(const int _notify_size)
	: is_queue_alive(true)
	, notify_size(_notify_size)
	{
	}

	void push(const T& element) override
	{
		queue.push(element);
		if(shouldNotify())
			condition.notify_one();
	}
	
	void push(T&& element) override
	{
		queue.push(std::move(element));
		if(shouldNotify())
			condition.notify_one();
	}
	
	void pop() override
	{
		queue.pop();
	}
	
	void popOnSuccses(const std::function<bool(const T&)>& function) override
	{
		queue.popOnSuccses(function);
	}
	
	void consumeAll(const std::function<void(const T&)>& function) override
	{
		queue.consumeAll(function);
	}
	
	void blockingConsumeAll(const std::function<void(const T&)>& function) override
	{
		std::unique_lock<std::mutex> lock(locker);
		condition.wait(lock, [&]() {
			return (!is_queue_alive.load(std::memory_order_acquire) || !shouldNotify());
		});
		consumeAll(function);
	}

	void notifyReaders() override
	{
		is_queue_alive.store(false, std::memory_order_acquire);
		condition.notify_all();
	}
	
	const int getSize() const override
	{
		queue.getSize();
	}

private:

	bool shouldNotify() const
	{
		return notify_size <= getSize();			
	}

	std::mutex locker;
	std::condition_variable condition;
	std::atomic<bool> is_queue_alive;
	const int notify_size;
	QueueType queue;

};

#endif
