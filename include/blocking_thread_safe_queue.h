#ifndef BLOCKINGTHREADSAFEQUEUE_H_
#define BLOCKINGTHREADSAFEQUEUE_H_

#include <condition_variable>
#include <mutex>
#include <atomic>

using size_t = std::size_t;

template<typename T, typename QueueType>
class BlockingThreadSafeQueue
{

public:
	BlockingThreadSafeQueue(const size_t& _notify_size)
	: is_queue_alive(true)
	, notify_size(_notify_size)
	{
	}

	bool push(const T& element)
	{
		bool result = queue.push(element);
		if(unlikely(shouldNotify() && result))
			condition.notify_one();
	}
	
	bool push(T&& element)
	{
		bool result = queue.push(std::move(element));
		if(unlikely(shouldNotify() && result))
			condition.notify_one();
	}
	
	bool pop()
	{
		return queue.pop();
	}
	
	bool popOnSuccses(const std::function<bool(const T&)>& function)
	{
		return queue.popOnSuccses(function);
	}
	
	void consumeAll(const std::function<void(const T&)>& function)
	{
		queue.consumeAll(function);
	}
	
	void blockingConsumeAll(const std::function<void(const T&)>& function)
	{
		std::unique_lock<std::mutex> lock(locker);
		condition.wait(lock, [&]() {
			return (!is_queue_alive.load(std::memory_order_acquire) || !shouldNotify());
		});
		consumeAll(function);
	}

	void notifyReaders() override
	{
		is_queue_alive.store(false, std::memory_order_release);
		condition.notify_all();
	}
	
	const size_t getSize() const override
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
	const size_t notify_size;
	QueueType queue;

};

#endif
