#pragma once
#include <memory>
#include <deque>
#include <mutex>
#include <condition_variable>

template<typename T>
using ptrDeque = std::deque<const T*>;

template<typename T>
class ThreadSafeQueue
{
public:
	ThreadSafeQueue(int max_size);
	virtual ~ThreadSafeQueue();

	void push(const T&);
	T pop();
	std::unique_ptr<ptrDeque<T>> getQueue();
	void blockUntilSizeReached();
	void notifyReader();
	bool isEmpty() const { return queue->empty(); }

private:

	void initalize_queue(std::unique_ptr<ptrDeque<T>>&& = std::make_unique<ptrDeque<T>>());

	bool is_queue_alive;
	std::mutex locker;
	std::mutex condition_mutex;
	std::condition_variable condition;
	std::unique_ptr<ptrDeque<T>> queue;
	int queue_size;
	const int max_size;
};

template<typename T>
ThreadSafeQueue<T>::ThreadSafeQueue(int max_size)
	: is_queue_alive(true)
	, queue_size(0)
	, max_size(max_size)
{
	initalize_queue();
}


template<typename T>
ThreadSafeQueue<T>::~ThreadSafeQueue()
{
	while (!queue->empty())
		pop();
}

template<typename T>
void ThreadSafeQueue<T>::push(const T& element)
{
	{
		std::lock_guard<std::mutex> lock(locker);
		const T * value = new T(element);
		queue->push_back(value);
		queue_size += 4;
	}
	if (queue_size >= max_size)
		condition.notify_one();
}

template<typename T>
T ThreadSafeQueue<T>::pop()
{
	const T* value;
	{
		std::lock_guard<std::mutex> lock(locker);
		value = queue->front();
		queue->pop_front();
	}
	T returned_element(std::move(*value));
	delete value;
	queue_size -= 4;
	return returned_element;
}

template<typename T>
std::unique_ptr<ptrDeque<T>> ThreadSafeQueue<T>::getQueue()
{
	if (!queue->empty())
	{
		auto new_queue = std::make_unique<ptrDeque<T>>();
		std::lock_guard<std::mutex> lock(locker);
		auto moved_queue = std::move(queue);
		initalize_queue(std::move(new_queue));
		return std::move(moved_queue);
	}
	return nullptr;
}

template<typename T>
void ThreadSafeQueue<T>::blockUntilSizeReached()
{
	std::unique_lock<std::mutex> lock(condition_mutex);
	while (queue_size < max_size && is_queue_alive)
		condition.wait(lock);
}

template<typename T>
void ThreadSafeQueue<T>::notifyReader()
{
	is_queue_alive = false;
	condition.notify_one();
}

template<typename T>
void ThreadSafeQueue<T>::initalize_queue(std::unique_ptr<ptrDeque<T>>&& _queue)
{
	queue_size = 0;
	queue = std::move(_queue);
}
