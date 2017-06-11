#ifndef BLOCKINGTHREADSAFEQUEUE_H_
#define BLOCKINGTHREADSAFEQUEUE_H_

#include <condition_variable>
#include <mutex>
#include <atomic>

template<typename T, typename QueueType>
class BlockingThreadSafeQueue : public IBlockThreadSafeQueue<T>
{
	BlockingThreadSafeQueue()
	: is_queue_alive(true)
	{
	}

	bool push(const T& element)
	{
		queue.push(element);
		condition.notify_one();
	}
	
	bool push(T&& element)
	{
		queue.push(std::move(element));
		condition.notify_one();
	}
	
	void pop()
	{
		queue.pop();
	}
	
	void popOnSuccsess(const std::function<bool(const T*)>& function)
	{
		queue.popOnSuccsess(function);
	}
	
	std::unique_ptr<IThreadSafeQueueIterator<T>> popElements()
	{
		queue.popElements();
	}
		
	std::unique_ptr<IThreadSafeQueueIterator<T>> blockingPopElements()
	{
		std::unique_lock<std::mutex> lock(locker);
		condition.wait(lock, [&]() {
			return !is_queue_alive.load(std::memory_order_acquire);
		});
		return popElements();
	}
	
	void consumeAll(const std::function<void(const T&)>& function)
	{
		queue.consumeAll(function);
	}
	
	void blockingConsumeAll(const std::function<void(const T&)>& function)
	{
		std::unique_lock<std::mutex> lock(locker);
		condition.wait(lock, [&]() {
			return !is_queue_alive.load(std::memory_order_acquire);
		});
		consumeAll(function);
	}
	
	bool isEmpty() const
	{
		queue.isEmpty();
	}

private:

	std::mutex locker;
	std::condition_variable condition;
	std::atomic<bool> is_queue_alive;
	QueueType queue;

};

#endif